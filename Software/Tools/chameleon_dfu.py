#!/usr/bin/env python3
"""Portable replacement for BOOT_LOADER_EXE.exe.

BOOT_LOADER_EXE.exe is dfu-programmer 0.7.1 with main() replaced by a
hard-coded four step sequence.  See report.md for the full reverse engineering
notes.

This slimmed-down build keeps only the Intel-HEX flashing path.  What reaches
the device is the *scrambled* image

    wire[addr] = image[addr] ^ ((addr + 0x2D) & 0xFF)

which the RevE-Rebooted boot loader de-scrambles while programming (the repo's
Firmware/ChameleonMini/Makefile `dfu-prog` target relies on the same thing).
The vendor's AES-128-ECB .bin detour (Createbin.exe / flash.bat) is not
reproduced here; flashing goes straight from Intel HEX.

Usage
-----
  chameleon_dfu.py flash-hex FW.hex FW.eep       # straight from Intel HEX
  chameleon_dfu.py erase | reset | launch | info | dump

Requires pyusb only.
"""

import argparse
import sys
import time

# --------------------------------------------------------------------------- #
# Address keyed scrambling.  Applied by Createbin.exe and undone by the boot
# loader running on the ATxmega while it programs (and re-applied on read back,
# which is why the vendor tool's read-back validation succeeds).
# --------------------------------------------------------------------------- #

SCRAMBLE_SEED = 0x2D          # low byte of the literal 12333 used by Createbin


def scramble(data, base_addr=0):
    """Symmetric: applying it twice is the identity."""
    return bytes(b ^ ((base_addr + i + SCRAMBLE_SEED) & 0xFF) for i, b in enumerate(data))


# --------------------------------------------------------------------------- #
# Target description (dfu-programmer 0.7.1 target_map entry + assign_target)
# --------------------------------------------------------------------------- #

# Every target BOOT_LOADER_EXE.exe knows is an ADC_XMEGA device (device_type 8),
# so dfu-programmer's device_type dispatch is not reproduced here: only the
# XMEGA branch of each command is kept.  What the ADC_8051 / ADC_AVR / ADC_AVR32
# branches did is recorded in report.md, section 7.


class Target:
    """Mirrors the fields assign_target() fills in programmer_arguments."""

    def __init__(self, name, vendor_id, chip_id, memory_size,
                 bootloader_size, bootloader_at_highmem, flash_page_size,
                 initial_abort, honor_interfaceclass,
                 eeprom_page_size, eeprom_memory_size):
        self.name = name
        self.vendor_id = vendor_id
        self.chip_id = chip_id
        self.flash_page_size = flash_page_size
        self.initial_abort = initial_abort
        self.honor_interfaceclass = honor_interfaceclass
        self.eeprom_page_size = eeprom_page_size
        self.eeprom_memory_size = eeprom_memory_size

        self.memory_address_top = memory_size - 1
        self.memory_address_bottom = 0
        self.flash_address_bottom = 0
        if bootloader_at_highmem:
            self.bootloader_bottom = memory_size - bootloader_size
            self.bootloader_top = self.memory_address_top
            self.flash_address_top = self.memory_address_top - bootloader_size
        else:
            self.bootloader_bottom = 0
            self.bootloader_top = bootloader_size - 1
            self.flash_address_top = self.memory_address_top
            self.flash_address_bottom = bootloader_size

        # The RevE-Rebooted boot loader owns the very top of the application
        # section.  Its CHIP ERASE runs NVM ERASE_APP over the whole application
        # section and then stamps a 16 byte marker back into 0x7FF0..0x7FFF (on
        # the 32a4u) via an erase+write of the top flash page - so right after an
        # erase that page is NOT blank.  Reserve the whole top flash page so that:
        #   * the post-erase blank check and the read-back validation stop below
        #     the marker instead of failing on it, and
        #   * application data can never land in that page and get wiped when the
        #     boot loader re-stamps the marker.
        # Everything below (incl. 0x7000..0x7EFF on the 32a4u) stays usable; the
        # original vendor tool instead threw away all of 0x7000..0x7FFF.
        self.flash_marker_bottom = self.flash_address_top - flash_page_size + 1
        self.flash_address_top -= flash_page_size


# NOTE on memory_size: on XMEGA the boot loader lives in a SEPARATE boot section
# that sits ABOVE the application flash, not in the top of it.  dfu-programmer's
# generic "bootloader eats the top N KB" model (memory_size = application size,
# then flash_address_top = memory_size - bootloader_size) therefore wrongly
# steals the top bootloader_size bytes of the application section - for the
# atxmega32a4u that blocked 0x7000..0x7FFF.  memory_size here is the TRUE total
# (application + boot section = app_size + bootloader_size), so that
# flash_address_top lands on the real application top (0x7FFF for the 32a4u) and
# the full application section stays writable.
TARGETS = {
    # the Chameleon Mini RevE (Rebooted) MCU - the only target BOOT_LOADER_EXE uses
    "atxmega32a4u": Target("atxmega32a4u", 0x03EB, 0x2FE4,
                           0x9000, 0x1000, True, 256, True, False, 32, 0x0400),
    "atxmega16a4u": Target("atxmega16a4u", 0x03EB, 0x2FE3,
                           0x5000, 0x1000, True, 256, True, False, 32, 0x0400),
    "atxmega64a4u": Target("atxmega64a4u", 0x03EB, 0x2FDD,
                           0x11000, 0x1000, True, 256, True, False, 32, 0x0800),
    "atxmega128a4u": Target("atxmega128a4u", 0x03EB, 0x2FE5,
                            0x22000, 0x2000, True, 256, True, False, 32, 0x0800),
}

DEFAULT_TARGET = "atxmega32a4u"


# --------------------------------------------------------------------------- #
# DFU transport (dfu.c)
# --------------------------------------------------------------------------- #

DFU_TIMEOUT = 20000            # ms, DFU_TIMEOUT in dfu.c

DFU_DETACH, DFU_DNLOAD, DFU_UPLOAD = 0, 1, 2
DFU_GETSTATUS, DFU_CLRSTATUS, DFU_GETSTATE, DFU_ABORT = 3, 4, 5, 6

OUT_REQ = 0x21                 # host->device, class, interface
IN_REQ = 0xA1                  # device->host, class, interface

DFU_STATES = [
    "appIDLE", "appDETACH", "dfuIDLE", "dfuDNLOAD-SYNC", "dfuDNBUSY",
    "dfuDNLOAD-IDLE", "dfuMANIFEST-SYNC", "dfuMANIFEST",
    "dfuMANIFEST-WAIT-RESET", "dfuUPLOAD-IDLE", "dfuERROR",
]
DFU_STATUSES = [
    "OK", "errTARGET", "errFILE", "errWRITE", "errERASE", "errCHECK_ERASED",
    "errPROG", "errVERIFY", "errADDRESS", "errNOTDONE", "errFIRMWARE",
    "errVENDOR", "errUSBR", "errPOR", "errUNKNOWN", "errSTALLEDPKT",
]

STATE_APP_IDLE, STATE_APP_DETACH, STATE_DFU_IDLE = 0, 1, 2
STATE_DFU_DNLOAD_SYNC, STATE_DFU_DNBUSY, STATE_DFU_DNLOAD_IDLE = 3, 4, 5
STATE_DFU_MANIFEST_SYNC, STATE_DFU_MANIFEST = 6, 7
STATE_DFU_MANIFEST_WAIT_RESET, STATE_DFU_UPLOAD_IDLE, STATE_DFU_ERROR = 8, 9, 10

STATUS_OK, STATUS_ERROR_CHECK_ERASED, STATUS_ERROR_NOTDONE = 0, 5, 9


def state_name(s):
    return DFU_STATES[s] if 0 <= s < len(DFU_STATES) else "unknown state"


def status_name(s):
    return DFU_STATUSES[s] if 0 <= s < len(DFU_STATUSES) else "unknown status"


class DfuError(Exception):
    pass


class DfuStatus:
    __slots__ = ("bStatus", "bwPollTimeout", "bState", "iString")

    def __init__(self, raw):
        self.bStatus = raw[0]
        self.bwPollTimeout = raw[1] | (raw[2] << 8) | (raw[3] << 16)
        self.bState = raw[4]
        self.iString = raw[5]

    def __repr__(self):
        return "<DfuStatus %s(0x%02x) %s(0x%02x) poll=%dms>" % (
            status_name(self.bStatus), self.bStatus,
            state_name(self.bState), self.bState, self.bwPollTimeout)


class DfuDevice:
    """dfu_device_t plus the dfu_* transport calls."""

    def __init__(self, handle, interface, debug=0):
        self.handle = handle            # usb.core.Device
        self.interface = interface
        self.debug = debug
        # dfu.c keeps `transaction` in a file static; it is never reset, which
        # is why it lives on the device object here rather than per transfer.
        self.transaction = 0

    def _log(self, level, msg):
        if level < self.debug:
            print(msg, file=sys.stderr)

    # -- dfu_download ------------------------------------------------------ #
    def download(self, data):
        wvalue = self.transaction & 0xFFFF
        self.transaction = (self.transaction + 1) & 0xFFFF
        n = self.handle.ctrl_transfer(OUT_REQ, DFU_DNLOAD, wvalue,
                                      self.interface, data or None, DFU_TIMEOUT)
        self._log(200, "dfu_download(%u) -> %d" % (len(data or b""), n))
        return n

    # -- dfu_upload -------------------------------------------------------- #
    def upload(self, length):
        wvalue = self.transaction & 0xFFFF
        self.transaction = (self.transaction + 1) & 0xFFFF
        buf = self.handle.ctrl_transfer(IN_REQ, DFU_UPLOAD, wvalue,
                                        self.interface, length, DFU_TIMEOUT)
        self._log(200, "dfu_upload(%u) -> %d" % (length, len(buf)))
        return bytes(buf)

    # -- dfu_get_status ---------------------------------------------------- #
    def get_status(self):
        raw = self.handle.ctrl_transfer(IN_REQ, DFU_GETSTATUS, 0,
                                        self.interface, 6, DFU_TIMEOUT)
        if len(raw) != 6:
            raise DfuError("short DFU_GETSTATUS (%d bytes)" % len(raw))
        st = DfuStatus(bytes(raw))
        self._log(100, "  %r" % (st,))
        return st

    def clear_status(self):
        self.handle.ctrl_transfer(OUT_REQ, DFU_CLRSTATUS, 0,
                                  self.interface, None, DFU_TIMEOUT)

    def abort(self):
        self.handle.ctrl_transfer(OUT_REQ, DFU_ABORT, 0,
                                  self.interface, None, DFU_TIMEOUT)

    def detach(self, timeout=0):
        self.handle.ctrl_transfer(OUT_REQ, DFU_DETACH, timeout,
                                  self.interface, None, DFU_TIMEOUT)

    def close(self):
        try:
            import usb.util
            usb.util.release_interface(self.handle, self.interface)
        except Exception:
            pass


def _find_dfu_interface(dev, honor_interfaceclass):
    """dfu_find_interface(): configuration 0 only."""
    cfg = dev[0]
    for intf in cfg:
        if honor_interfaceclass:
            if intf.bInterfaceClass == 0xFE and intf.bInterfaceSubClass == 0x01:
                return intf.bInterfaceNumber
        else:
            # upstream: "if they don't care about the class, just return the first one"
            return intf.bInterfaceNumber
    return None


def dfu_make_idle(dev, initial_abort):
    """dfu_make_idle(): 0 = now idle, 1 = caller should re-enumerate."""
    if initial_abort:
        try:
            dev.abort()
        except Exception:
            pass
    for _ in range(4):
        try:
            st = dev.get_status()
        except Exception:
            try:
                dev.clear_status()
            except Exception:
                pass
            continue
        dev._log(100, "State: %s (%d)" % (state_name(st.bState), st.bState))
        if st.bState == STATE_DFU_IDLE:
            if st.bStatus == STATUS_OK:
                return 0
            dev.clear_status()
        elif st.bState == STATE_APP_IDLE:
            dev.detach(0)
        elif st.bState in (STATE_APP_DETACH, STATE_DFU_MANIFEST_WAIT_RESET):
            dev.handle.reset()
            return 1
        elif st.bState == STATE_DFU_ERROR:
            dev.clear_status()
        else:
            dev.abort()
    raise DfuError("could not transition the device into dfuIDLE")


def dfu_device_init(target, bus=0, dev_addr=0, debug=0):
    """dfu_device_init(): find, open, claim and idle the DFU device."""
    import usb.core
    import usb.util

    for _attempt in range(4):
        dev = None
        for cand in usb.core.find(find_all=True,
                                  idVendor=target.vendor_id,
                                  idProduct=target.chip_id):
            if bus and not (cand.bus == bus and cand.address == dev_addr):
                continue
            dev = cand
            break
        if dev is None:
            return None

        intf = _find_dfu_interface(dev, target.honor_interfaceclass)
        if intf is None:
            raise DfuError("failed to find the DFU interface")

        # Only configure the device if it is not already in configuration 1.
        #
        # This is not cosmetic.  The factory RevE Rebooted boot loader arms a
        # write lock when it sees SET_CONFIGURATION: every following PROGRAM is
        # answered with a STALL on the control pipe plus DFU status errWRITE,
        # until the next CHIP ERASE clears the lock again.  Since the boot
        # loader sequence re-opens the device for every step, the erase from
        # step 1 is undone by step 2's SET_CONFIGURATION and the EEPROM write
        # dies with "[Errno 32] Pipe error".
        #
        # BOOT_LOADER_EXE.exe does not hit this because libusb-win32 turns
        # usb_set_configuration() into a no-op when the requested configuration
        # is already the active one, so the boot loader never sees the request.
        # Linux/libusb-1.0 does put it on the wire, which is why plain
        # dfu-programmer fails here in exactly the same place.
        try:
            active = dev.get_active_configuration().bConfigurationValue
        except usb.core.USBError:
            active = None
        if active != 1:
            try:
                dev.set_configuration(1)
            except usb.core.USBError:
                pass                               # matches upstream's soft failure path
        try:
            usb.util.claim_interface(dev, intf)
        except usb.core.USBError as exc:
            raise DfuError("failed to claim the DFU interface: %s" % exc)

        handle = DfuDevice(dev, intf, debug)
        if dfu_make_idle(handle, target.initial_abort) == 0:
            return handle
        usb.util.release_interface(dev, intf)       # re-enumerated, try again
    raise DfuError("device never reached dfuIDLE")


# --------------------------------------------------------------------------- #
# Sparse image buffer (atmel_buffer_out_t / intel_buffer_out_t)
# --------------------------------------------------------------------------- #

# dfu-programmer's return_codes_enum, as used by commands.c and main().
RC_SUCCESS = 0
RC_UNSPECIFIED_ERROR = 1
RC_ARGUMENT_ERROR = 2
RC_DEVICE_ACCESS_ERROR = 3
RC_BUFFER_INIT_ERROR = 4
RC_FLASH_READ_ERROR = 5
RC_FLASH_WRITE_ERROR = 6
RC_VALIDATION_ERROR_IN_REGION = 7
RC_VALIDATION_ERROR_OUTSIDE_REGION = 8

UNASSIGNED = 0xFFFF            # the 'no data here' marker used by dfu-programmer


class ImageBuffer:
    def __init__(self, total_size, page_size):
        if total_size <= 0 or page_size <= 0:
            raise ValueError("size must be > 0")
        self.total_size = total_size
        self.page_size = page_size
        self.data = [UNASSIGNED] * total_size
        self.data_start = None
        self.data_end = None
        self.valid_start = 0
        self.valid_end = total_size - 1
        self.block_start = 0
        self.block_end = 0

    # intel_process_data()
    def put(self, addr, value):
        if not 0 <= addr < self.total_size:
            return False
        self.data[addr] = value & 0xFF
        if self.data_start is None or addr < self.data_start:
            self.data_start = addr
        if self.data_end is None or addr > self.data_end:
            self.data_end = addr
        return True

    # atmel_flash_prep_buffer(): fill the holes of any page that holds data
    def prep(self):
        for base in range(0, self.valid_end, self.page_size):
            page = self.data[base:base + self.page_size]
            if any(v <= 0xFF for v in page):
                for i, v in enumerate(page):
                    if v > 0xFF:
                        self.data[base + i] = 0xFF

    # the rescan atmel_flash() does after prep()
    def rescan(self):
        self.data_start = None
        self.data_end = None
        for i, v in enumerate(self.data):
            if v <= 0xFF:
                if self.data_start is None:
                    self.data_start = i
                self.data_end = i


class ReadBuffer:
    """atmel_buffer_in_t - what a read-back lands in (plain bytes, 0xFF = erased)."""

    def __init__(self, total_size, page_size):
        self.total_size = total_size
        self.page_size = page_size
        self.data = bytearray(b"\xff" * total_size)
        self.data_start = 0
        self.data_end = 0
        self.block_start = 0
        self.block_end = 0


# --------------------------------------------------------------------------- #
# Image loaders
# --------------------------------------------------------------------------- #

def load_intel_hex(path, buf, scramble_image=True):
    """Read an Intel HEX file into `buf`.

    With scramble_image=True the bytes are stored pre-scrambled, i.e. exactly
    what Createbin.exe + BOOT_LOADER_EXE.exe would have put on the wire.
    """
    outside = 0
    ext_base = 0
    with open(path, "r") as fh:
        for lineno, line in enumerate(fh, 1):
            line = line.strip()
            if not line:
                continue
            if line[0] != ":":
                raise ValueError("%s:%d: not an Intel HEX record" % (path, lineno))
            raw = bytes.fromhex(line[1:])
            count, addr_hi, addr_lo, rtype = raw[0], raw[1], raw[2], raw[3]
            addr = (addr_hi << 8) | addr_lo
            payload = raw[4:4 + count]
            if (sum(raw) & 0xFF) != 0:
                raise ValueError("%s:%d: bad checksum" % (path, lineno))
            if rtype == 0x00:
                for i, b in enumerate(payload):
                    a = ext_base + addr + i
                    if scramble_image:
                        b ^= (a + SCRAMBLE_SEED) & 0xFF
                    if not buf.put(a, b):
                        outside += 1
            elif rtype == 0x01:
                break
            elif rtype == 0x02:
                ext_base = ((payload[0] << 8) | payload[1]) << 4
            elif rtype == 0x04:
                ext_base = ((payload[0] << 8) | payload[1]) << 16
            elif rtype in (0x03, 0x05):
                pass                                # start address records
            else:
                raise ValueError("%s:%d: record type 0x%02X unknown"
                                 % (path, lineno, rtype))
    return outside


def load_raw_bin(path, buf, base=0, scramble_image=True):
    with open(path, "rb") as fh:
        blob = fh.read()
    outside = 0
    for i, b in enumerate(blob):
        a = base + i
        if scramble_image:
            b ^= (a + SCRAMBLE_SEED) & 0xFF
        if not buf.put(a, b):
            outside += 1
    return outside


# --------------------------------------------------------------------------- #
# Atmel DFU command layer (atmel.c)
# --------------------------------------------------------------------------- #

ATMEL_MAX_TRANSFER_SIZE = 0x400
ATMEL_MAX_FLASH_BUFFER_SIZE = 1168
ATMEL_CONTROL_BLOCK_SIZE = 64   # ATMEL_AVR32_CONTROL_BLOCK_SIZE; 32 on GRP_AVR
ATMEL_FOOTER_SIZE = 16
ATMEL_64KB_PAGE = 0x10000
ATMEL_ERASE_ALL = 0xFF

MEM_FLASH, MEM_EEPROM, MEM_SECURITY, MEM_CONFIG = 0, 1, 2, 3
MEM_BOOT, MEM_SIG, MEM_USER = 4, 5, 6
MEM_UNIT_NAMES = ["flash", "eeprom", "security", "config", "bootloader",
                  "signature", "user", "int_ram", "ext_cs0", "ext_cs1",
                  "ext_cs2", "ext_cs3", "ext_cs4", "ext_cs5", "ext_cs6",
                  "ext_cs7", "ext_df"]

# Constant 16 byte DFU suffix emitted by atmel_flash_populate_footer() with
# vendorId = productId = bcdFirmware = 0xFFFF and crc = 0.
ATMEL_FOOTER = bytes([0x00, 0x00, 0x00, 0x00,
                      0x10, ord("D"), ord("F"), ord("U"),
                      0x01, 0x10, 0xFF, 0xFF,
                      0xFF, 0xFF, 0xFF, 0xFF])


def _check_status(dev, what):
    st = dev.get_status()
    if st.bStatus != STATUS_OK:
        if st.bState == STATE_DFU_ERROR:
            dev.clear_status()
        raise DfuError("%s: status %s (0x%02x)"
                       % (what, status_name(st.bStatus), st.bStatus))
    return st


def atmel_select_memory_unit(dev, unit):
    """[0x06, 0x03, 0x00, unit]."""
    if unit > 16:
        raise DfuError("valid memory units are 0..0x10, not 0x%X" % unit)
    dev._log(50, "Selecting %s memory unit." % MEM_UNIT_NAMES[unit])
    dev.download(bytes([0x06, 0x03, 0x00, unit]))
    _check_status(dev, "atmel_select_memory_unit")


def atmel_select_page(dev, page):
    """[0x06, 0x03, 0x01, hi, lo] - the 64kB page select."""
    page &= 0xFFFF
    dev._log(50, "Selecting page %d, address 0x%X." % (page, page << 16))
    dev.download(bytes([0x06, 0x03, 0x01, (page >> 8) & 0xFF, page & 0xFF]))
    _check_status(dev, "atmel_select_page")


def atmel_erase_flash(dev, mode=ATMEL_ERASE_ALL, quiet=False):
    """[0x04, 0x00, mode] then poll while errNOTDONE/dfuDNBUSY."""
    if not quiet:
        sys.stderr.write("Erasing flash...  ")
        sys.stderr.flush()
    dev.download(bytes([0x04, 0x00, mode]))
    started = time.time()
    retries = 0
    while True:
        try:
            st = dev.get_status()
        except Exception:
            dev.clear_status()
            retries += 1
            if retries >= 10:
                raise DfuError("CMD_ERASE status check failed %d times" % retries)
        else:
            if not (st.bStatus == STATUS_ERROR_NOTDONE
                    and st.bState == STATE_DFU_DNBUSY):
                if not quiet:
                    sys.stderr.write("Success\n")
                return st.bStatus
            time.sleep(0.1)
        if time.time() - started >= 20:
            if not quiet:
                sys.stderr.write("ERROR\n")
            raise DfuError("CMD_ERASE time limit 20s exceeded")


def _atmel_blank_page_check(dev, start, end):
    """[0x03, 0x01, s_hi, s_lo, e_hi, e_lo]; returns 0 or first_nonblank+1."""
    if start > end:
        raise DfuError("end address 0x%X before start 0x%X" % (end, start))
    if end >= ATMEL_64KB_PAGE:
        raise DfuError("address out of 64kB page range")
    dev.download(bytes([0x03, 0x01,
                        (start >> 8) & 0xFF, start & 0xFF,
                        (end >> 8) & 0xFF, end & 0xFF]))
    st = dev.get_status()
    if st.bStatus == STATUS_OK:
        return 0
    if st.bStatus == STATUS_ERROR_CHECK_ERASED:
        raw = dev.upload(2)
        if len(raw) != 2:
            raise DfuError("__atmel_blank_page_check DFU_UPLOAD failed")
        return ((raw[0] << 8) | raw[1]) + 1
    if st.bState == STATE_DFU_ERROR:
        dev.clear_status()
    raise DfuError("blank check: status %s" % status_name(st.bStatus))


def atmel_blank_check(dev, start, end, quiet=False):
    """0 = blank, >0 = first non-blank address."""
    if start > end:
        raise DfuError("end address 0x%X before start 0x%X" % (end, start))
    atmel_select_memory_unit(dev, MEM_FLASH)
    if not quiet:
        sys.stderr.write("Checking memory from 0x%X to 0x%X...  " % (start, end))
        sys.stderr.flush()
    cur = start
    while True:
        page = cur >> 16
        page_end = min((page << 16) + 0xFFFF, end)
        atmel_select_page(dev, page)
        res = _atmel_blank_page_check(dev, cur & 0xFFFF, page_end & 0xFFFF)
        if res != 0:
            addr = res + (page << 16)
            if not quiet:
                sys.stderr.write("Not blank at 0x%X.\n" % addr)
            return addr
        if page_end + 1 >= end:
            if not quiet:
                sys.stderr.write("Empty.\n")
            return 0
        cur = page_end + 1


def _atmel_flash_block(dev, buf):
    """One DFU_DNLOAD covering buf.block_start .. buf.block_end."""
    length = buf.block_end - buf.block_start + 1
    if buf.block_start > buf.block_end:
        raise DfuError("end address before start address")
    if length > ATMEL_MAX_TRANSFER_SIZE:
        raise DfuError("0x%X byte message > MAX TRANSFER SIZE (0x%X)"
                       % (length, ATMEL_MAX_TRANSFER_SIZE))

    msg = bytearray(ATMEL_MAX_FLASH_BUFFER_SIZE)
    data_off = ATMEL_CONTROL_BLOCK_SIZE + buf.block_start % ATMEL_CONTROL_BLOCK_SIZE

    start = buf.block_start % ATMEL_64KB_PAGE
    end = buf.block_end % ATMEL_64KB_PAGE
    msg[0] = 0x01
    # header[1] is the eeprom flag, but on XMEGA it stays 0x00: the unit was
    # already chosen by atmel_flash() via atmel_select_memory_unit().
    msg[1] = 0x00
    msg[2] = (start >> 8) & 0xFF
    msg[3] = start & 0xFF
    msg[4] = (end >> 8) & 0xFF
    msg[5] = end & 0xFF

    for i in range(length):
        msg[data_off + i] = buf.data[buf.block_start + i] & 0xFF
    msg[data_off + length:data_off + length + ATMEL_FOOTER_SIZE] = ATMEL_FOOTER

    message_length = data_off + length + ATMEL_FOOTER_SIZE
    try:
        sent = dev.download(bytes(msg[:message_length]))
    except Exception as exc:
        # __atmel_flash_block() @ 0x004046D0 turns libusb's -EPIPE into
        # "Device is write protected."  On the factory boot loader that is
        # literally what happened: a SET_CONFIGURATION armed its write lock,
        # and only a CHIP ERASE clears it - see dfu_device_init().
        if getattr(exc, "errno", None) == 32:
            raise DfuError("Device is write protected (the boot loader stalled "
                           "the write; a chip erase re-enables programming)")
        raise
    if sent != message_length:
        raise DfuError("flash data dfu_download failed: expected %d, got %d"
                       % (message_length, sent))
    st = dev.get_status()
    if st.bStatus != STATUS_OK:
        if st.bState == STATE_DFU_ERROR:
            dev.clear_status()
        raise DfuError("page write failed (%s)" % status_name(st.bStatus))


def atmel_flash(dev, buf, eeprom=False, quiet=False):
    """atmel_flash() @ 0x00404090 - the page walk, including its console output.

    Output shape (debug <= 50), matching the vendor tool byte for byte:
        "0%                            100%  Programming 0x<n> bytes...\n"
        "[" ">" * 32 "]  Success\n"
    and on failure " X  ERROR\n" plus a "Memory ... error" line.
    """
    if buf.valid_start > buf.valid_end:
        raise DfuError("no valid target memory, end 0x%X before start 0x%X"
                       % (buf.valid_end, buf.valid_start))
    buf.prep()
    buf.rescan()
    if buf.data_start is None:
        raise DfuError("no valid data to flash")
    if buf.data_start < buf.valid_start or buf.data_end > buf.valid_end:
        raise DfuError("data exists outside of the valid target flash region "
                       "(0x%X..0x%X, valid 0x%X..0x%X)"
                       % (buf.data_start, buf.data_end,
                          buf.valid_start, buf.valid_end))

    atmel_select_memory_unit(dev, MEM_EEPROM if eeprom else MEM_FLASH)

    if not quiet:
        if dev.debug <= 50:
            sys.stderr.write("0%                            100%  ")
        sys.stderr.write("Programming 0x%X bytes...\n"
                         % (buf.data_end - buf.data_start + 1))
        if dev.debug <= 50:
            sys.stderr.write("[")
        sys.stderr.flush()

    progress = [0]
    buf.block_start = buf.data_start
    page = buf.block_start >> 16
    try:
        atmel_select_page(dev, page)

        while buf.block_start <= buf.data_end:
            if (buf.block_start >> 16) != page:
                page = buf.block_start >> 16
                atmel_select_page(dev, page)

            # grow the block while the data stays contiguous, inside the 64kB
            # page and within ATMEL_MAX_TRANSFER_SIZE
            end = buf.block_start
            while (end <= buf.data_end
                   and buf.data[end] <= 0xFF
                   and (end - buf.block_start + 1) <= ATMEL_MAX_TRANSFER_SIZE
                   and (end >> 16) == page):
                end += 1
            buf.block_end = end - 1

            _atmel_flash_block(dev, buf)

            # skip forward over unassigned entries
            nxt = buf.block_end + 1
            while nxt <= buf.data_end and buf.data[nxt] > 0xFF:
                nxt += 1
            buf.block_start = nxt
            if not quiet:
                _print_progress_bar(buf, progress, dev.debug)
    except DfuError:
        if not quiet:
            if dev.debug <= 50:
                sys.stderr.write(" X  ")
            sys.stderr.write("ERROR\n")
            sys.stderr.write("Memory write error, use debug for more info.\n")
        raise

    if not quiet:
        if dev.debug <= 50:
            sys.stderr.write("]  ")
        sys.stderr.write("Success\n")
        sys.stderr.flush()


def _atmel_read_block(dev, start, end):
    """[0x03, 0x00, s_hi, s_lo, e_hi, e_lo] then DFU_UPLOAD.

    The flag stays 0x00 even for EEPROM (it is 0x02 only on GRP_AVR), because
    the unit was already chosen by atmel_select_memory_unit().
    """
    if start > end:
        raise DfuError("start address is after end address")
    length = end - start + 1
    if length > ATMEL_MAX_TRANSFER_SIZE:
        raise DfuError("transfer size must not exceed 0x%X" % ATMEL_MAX_TRANSFER_SIZE)
    dev.download(bytes([0x03, 0x00,
                        (start >> 8) & 0xFF, start & 0xFF,
                        (end >> 8) & 0xFF, end & 0xFF]))
    data = dev.upload(length)
    if len(data) != length:
        raise DfuError("dfu_upload returned %d of %d bytes" % (len(data), length))
    return data


def atmel_read_flash(dev, buin, mem_segment=MEM_FLASH, quiet=False):
    """atmel_read_flash() @ 0x00402EB0.

    Reads buin.data_start .. buin.data_end into buin.data in 0x400 byte chunks.
    Returns 0 on success or a negative error code, and - like the original -
    prints nothing at all on failure: the caller sees only
    "Reading 0x%X bytes..." with no trailing "Success".  That silence is exactly
    what the vendor tool's log shows when the device refuses a read.
    """
    if buin is None or dev is None:
        if not quiet:
            sys.stderr.write("Program Error, use debug for more info.\n")
        return -1
    if mem_segment not in (MEM_FLASH, MEM_EEPROM, MEM_USER):
        dev._log(50, "Invalid memory segment %d to read." % mem_segment)
        if not quiet:
            sys.stderr.write("Program Error, use debug for more info.\n")
        return -1
    try:
        atmel_select_memory_unit(dev, mem_segment)
    except DfuError:
        dev._log(50, "Error selecting memory unit.")
        if not quiet:
            sys.stderr.write("Memory access error, use debug for more info.\n")
        return -3

    if not quiet:
        if dev.debug <= 50:
            sys.stderr.write("0%                            100%  ")
        sys.stderr.write("Reading 0x%X bytes...\n"
                         % (buin.data_end - buin.data_start + 1))
        sys.stderr.flush()

    buin.block_start = buin.data_start
    page = buin.data_start >> 16
    # the user page is not paged
    if mem_segment != MEM_USER:
        try:
            atmel_select_page(dev, page)
        except DfuError:
            dev._log(50, "ERROR selecting 64kB page.")
            return -3

    progress = [0]
    retval = 0
    while buin.block_start <= buin.data_end:
        if (buin.block_start >> 16) != page:
            page = buin.block_start >> 16
            try:
                atmel_select_page(dev, page)
            except DfuError:
                dev._log(50, "ERROR selecting 64kB page.")
                # NOTE: upstream only DEBUGs here and keeps going
        buin.block_end = buin.block_start + ATMEL_MAX_TRANSFER_SIZE - 1
        if (buin.block_end >> 16) > page:
            # verbatim from the binary; for <= 64 kB targets `page` is always 0
            # so this evaluates to 0xFFFFFFFF and is clamped by the next test.
            buin.block_end = (page << 16) - 1
        if buin.block_end > buin.data_end:
            buin.block_end = buin.data_end
        try:
            chunk = _atmel_read_block(dev, buin.block_start & 0xFFFF,
                                      buin.block_end & 0xFFFF)
        except (DfuError, Exception):
            retval = -5
            break
        buin.data[buin.block_start:buin.block_start + len(chunk)] = chunk
        buin.block_start = buin.block_end + 1
        if not quiet:
            _print_progress_bar(buin, progress, dev.debug)

    if not quiet and retval == 0:
        sys.stderr.write("Success\n")
    return retval


def _print_progress_bar(info, progress, debug=0):
    """atmel_print_progress_bar() @ 0x00401E40 - 32 column bar of '>'.

    Suppressed when the debug level is above 50, exactly like the original.
    """
    if debug > 50:
        return
    span = info.data_end - info.data_start + 1
    if span <= 0:
        return
    while 32 * (info.block_end - info.data_start + 1) > progress[0]:
        sys.stderr.write(">")
        progress[0] += span
    sys.stderr.flush()


def atmel_validate_buffer(buin, bout, quiet=False):
    """atmel_validate_buffer() @ 0x00401EA0.

    Returns 0 on a clean match, -N for N mismatching bytes inside the data
    region, +N for N mismatching bytes outside it (where 0xFF was expected).
    """
    if not quiet:
        sys.stderr.write("Validating...  ")
        sys.stderr.flush()
    in_region = 0
    outside = 0
    for i in range(bout.valid_start, bout.valid_end + 1):
        want = bout.data[i]
        got = buin.data[i]
        if want > 0xFF:
            if got != 0xFF:
                outside += 1
        elif (want & 0xFF) != got:
            if in_region == 0 and not quiet:
                sys.stderr.write("ERROR\n")
            in_region += 1
    if not quiet:
        if in_region or outside:
            sys.stderr.write("%d invalid bytes in program region, "
                             "%d outside region.\n" % (in_region, outside))
        else:
            sys.stderr.write("Success\n")
    return -in_region if in_region else outside


def atmel_start_app_reset(dev):
    """[0x04, 0x03, 0x00] + zero length DNLOAD - watchdog reset."""
    dev.download(bytes([0x04, 0x03, 0x00]))
    dev.download(b"")


def atmel_start_app_noreset(dev):
    """[0x04, 0x03, 0x01, 0x00, 0x00] + zero length DNLOAD - plain jump."""
    dev.download(bytes([0x04, 0x03, 0x01, 0x00, 0x00]))
    dev.download(b"")


# The XMEGA rows of atmel_read_config()'s 19-entry table at 0x0040AE00, which
# is indexed by (data0, data1, device_map, name); the other 12 rows are 8051 or
# plain AVR only.  data0 is the memory unit: 0x04 = mem_boot, 0x05 = mem_sig.
ATMEL_READ_CONFIG_TABLE = [
    (0x04, 0x00, "Bootloader Version"),
    (0x04, 0x01, "Device boot ID 1"),
    (0x04, 0x02, "Device boot ID 2"),
    (0x05, 0x00, "Manufacturer Code"),
    (0x05, 0x01, "Family Code"),
    (0x05, 0x02, "Product Name"),
    (0x05, 0x03, "Product Revision"),
]


def atmel_read_command(dev, data0, data1):
    """atmel_read_command(), GRP_AVR32 branch.

    select_memory_unit(data0), then read the single byte at data1 out of that
    configuration memory unit.  (The GRP_AVR branch sends a 3 byte
    DFU_DNLOAD [0x05, data0, data1] instead; see report.md.)
    """
    atmel_select_memory_unit(dev, data0)
    return _atmel_read_block(dev, data1, data1)[0]


def atmel_read_config(dev):
    """atmel_read_config(): the table entries that apply to an XMEGA."""
    info = []
    for data0, data1, name in ATMEL_READ_CONFIG_TABLE:
        try:
            info.append((name, atmel_read_command(dev, data0, data1)))
        except Exception:
            info.append((name, None))
    return info


# --------------------------------------------------------------------------- #
# Command level (commands.c) and the hard-coded BOOT_LOADER_EXE sequence
# --------------------------------------------------------------------------- #

def execute_erase(dev, target, force=True, suppress_validation=False, quiet=False):
    """execute_erase() @ 0x00404A80 - `erase --force` in the preset argv.

    With --force the pre-erase blank check is skipped, so the observable order
    is "Erasing flash..." *before* "Checking memory from ...".  Without --force
    it is the other way round.
    """
    if not force:
        if atmel_blank_check(dev, target.flash_address_bottom,
                             target.flash_address_top, quiet) == 0:
            if not quiet:
                sys.stderr.write("Chip already blank, to force erase use --force.\n")
            return RC_SUCCESS
    dev._log(40, "erase 0x%X bytes."
             % (target.flash_address_top - target.flash_address_bottom))
    try:
        atmel_erase_flash(dev, ATMEL_ERASE_ALL, quiet)
    except DfuError:
        return RC_UNSPECIFIED_ERROR
    if not suppress_validation:
        return atmel_blank_check(dev, target.flash_address_bottom,
                                 target.flash_address_top, quiet)
    return RC_SUCCESS


def execute_validate(dev, bout, segment, quiet=False):
    """execute_validate() @ 0x00404C90.

    Reads back the **whole valid region**, not just the programmed range:

        buin.data_start = bout.valid_start
        buin.data_end   = bout.valid_end

    so for atxmega32a4u that is 0x400 bytes for EEPROM and the full 0x8000 byte
    application section for flash (0x0..0x7FFF).  A failed read yields
    RC_FLASH_READ_ERROR (5), which main() deliberately tolerates.
    """
    buin = ReadBuffer(bout.total_size, bout.page_size)
    buin.data_start = bout.valid_start
    buin.data_end = bout.valid_end
    r = atmel_read_flash(dev, buin, segment, quiet)
    if r != 0:
        dev._log(40, "ERROR: could not read memory, err %d." % r)
        return RC_FLASH_READ_ERROR
    v = atmel_validate_buffer(buin, bout, quiet)
    if v == 0:
        return RC_SUCCESS
    return (RC_VALIDATION_ERROR_IN_REGION if v < 0
            else RC_VALIDATION_ERROR_OUTSIDE_REGION)


def _buffer_for_segment(target, segment):
    if segment == MEM_EEPROM:
        if not target.eeprom_memory_size:
            raise DfuError("this device has no eeprom")
        return ImageBuffer(target.eeprom_memory_size, target.eeprom_page_size)
    if segment == MEM_USER:
        return ImageBuffer(target.flash_page_size, target.flash_page_size)
    return ImageBuffer(target.memory_address_top + 1, target.flash_page_size)


def execute_flash(dev, target, loader, segment=MEM_FLASH,
                  suppress_bootloader_mem=True, suppress_validation=False,
                  quiet=False):
    """execute_flash() @ 0x00404DF0 - `flash [--eeprom] <file>`.

    Returns a return_codes_enum value rather than raising, so that
    run_boot_loader_sequence() can reproduce main()'s tolerance of
    RC_FLASH_READ_ERROR.

    `loader` is called as loader(buf) and fills the ImageBuffer.
    """
    buf = _buffer_for_segment(target, segment)
    loader(buf)

    if segment == MEM_FLASH:
        buf.valid_start = target.flash_address_bottom
        buf.valid_end = target.flash_address_top
        for a in range(target.bootloader_bottom,
                       min(target.bootloader_top, buf.total_size - 1) + 1):
            if buf.data[a] <= 0xFF:
                if not suppress_bootloader_mem:
                    sys.stderr.write("Bootloader and code overlap.\n")
                    sys.stderr.write("Use --suppress-bootloader-mem to ignore\n")
                    return RC_BUFFER_INIT_ERROR
                buf.data[a] = UNASSIGNED

    try:
        atmel_flash(dev, buf, eeprom=(segment == MEM_EEPROM), quiet=quiet)
    except DfuError as exc:
        dev._log(40, "Error writing memory data. (%s)" % exc)
        if not quiet:
            sys.stderr.write("Memory write error, use debug for more info.\n")
        return RC_FLASH_WRITE_ERROR

    if not suppress_validation:
        rc = execute_validate(dev, buf, segment, quiet)
        if rc != RC_SUCCESS:
            return rc

    if not quiet:
        written = buf.data_end - buf.data_start + 1
        span = buf.valid_end - buf.valid_start + 1
        sys.stderr.write("0x%X bytes written into 0x%X bytes memory (%.02f%%).\n"
                         % (written, span, 100.0 * written / span))
    return RC_SUCCESS


def run_boot_loader_sequence(target, flash_loader, eeprom_loader,
                             quiet=False, debug=0, suppress_validation=False,
                             bus=0, dev_addr=0):
    """The four steps main() @ 0x00407050 performs.

    Faithful to the original, including:
      * re-opening the USB device for every step
      * advancing only while the step returned RC_SUCCESS or RC_FLASH_READ_ERROR
      * printing "load_success!" when the *last* step returned RC_SUCCESS

    Returns main()'s process exit code (0 or 2).
    """
    steps = [
        ("erase --force",
         lambda d: execute_erase(d, target, force=True, quiet=quiet)),
        ("flash --eeprom myfilee.bin",
         lambda d: execute_flash(d, target, eeprom_loader, MEM_EEPROM,
                                 suppress_validation=suppress_validation,
                                 quiet=quiet)),
        ("flash myfile.bin",
         lambda d: execute_flash(d, target, flash_loader, MEM_FLASH,
                                 suppress_validation=suppress_validation,
                                 quiet=quiet)),
        ("reset",
         lambda d: atmel_start_app_reset(d) or RC_SUCCESS),
    ]

    print("old_driver_bootloader")
    rc = RC_SUCCESS
    for i, (name, fn) in enumerate(steps):
        dev = dfu_device_init(target, bus, dev_addr, debug)
        if dev is None:
            sys.stderr.write("%s: no device present.\n" % "dfu-old-driver")
            rc = RC_DEVICE_ACCESS_ERROR
            break
        try:
            dev._log(1000, "--- step %d: %s" % (i, name))
            rc = fn(dev)
        finally:
            dev.close()
        if rc not in (RC_SUCCESS, RC_FLASH_READ_ERROR):
            break
    if rc == RC_SUCCESS:
        print("load_success!")
    return 0


# --------------------------------------------------------------------------- #
# CLI
# --------------------------------------------------------------------------- #

def _target(args):
    try:
        return TARGETS[args.target]
    except KeyError:
        raise SystemExit("unsupported target %r (known: %s)"
                         % (args.target, ", ".join(sorted(TARGETS))))


def _hex_loader(path, scramble_image):
    def loader(buf):
        outside = load_intel_hex(path, buf, scramble_image)
        if outside:
            sys.stderr.write("WARNING: 0x%X bytes of %s are outside target "
                             "memory and will not be written.\n" % (outside, path))
    return loader


def cmd_flash_hex(args):
    """The portable path: Intel HEX straight in, no .bin round trip."""
    target = _target(args)
    return run_boot_loader_sequence(
        target,
        _hex_loader(args.hex_file, not args.no_scramble),
        _hex_loader(args.eep_file, not args.no_scramble),
        quiet=args.quiet, debug=args.debug,
        suppress_validation=args.suppress_validation,
        bus=args.bus, dev_addr=args.address)


def cmd_erase(args):
    target = _target(args)
    dev = dfu_device_init(target, args.bus, args.address, args.debug)
    if dev is None:
        raise SystemExit("no device present")
    try:
        return execute_erase(dev, target, force=args.force, quiet=args.quiet)
    finally:
        dev.close()


def _simple(fn):
    def run(args):
        target = _target(args)
        dev = dfu_device_init(target, args.bus, args.address, args.debug)
        if dev is None:
            raise SystemExit("no device present")
        try:
            return fn(dev, target, args)
        finally:
            dev.close()
    return run


def _do_info(dev, target, args):
    for name, val in atmel_read_config(dev):
        print("%-22s %s" % (name, "n/a" if val is None else "0x%02X (%d)" % (val, val)))
    return 0


def _do_dump(dev, target, args):
    seg = MEM_EEPROM if args.eeprom else MEM_FLASH
    if seg == MEM_EEPROM:
        start, end = 0, target.eeprom_memory_size - 1
        total, page = target.eeprom_memory_size, target.eeprom_page_size
    else:
        start, end = target.flash_address_bottom, target.flash_address_top
        total, page = target.memory_address_top + 1, target.flash_page_size
    buin = ReadBuffer(total, page)
    buin.data_start, buin.data_end = start, end
    r = atmel_read_flash(dev, buin, seg, quiet=args.quiet)
    if r != 0:
        sys.stderr.write("read failed (err %d) - this boot loader may refuse "
                         "%s reads; the vendor tool ignores the same failure.\n"
                         % (r, "eeprom" if seg == MEM_EEPROM else "flash"))
        return RC_FLASH_READ_ERROR
    data = bytes(buin.data[start:end + 1])
    if not args.no_scramble:
        data = scramble(data, start)
    with open(args.out, "wb") as fh:
        fh.write(data)
    sys.stderr.write("wrote 0x%X bytes to %s\n" % (len(data), args.out))
    return RC_SUCCESS


def main(argv=None):
    ap = argparse.ArgumentParser(
        description="Portable replacement for BOOT_LOADER_EXE.exe "
                    "(ChameleonMini RevE Rebooted DFU flasher)")
    ap.add_argument("--target", default=DEFAULT_TARGET,
                    help="device target (default %(default)s)")
    ap.add_argument("--quiet", action="store_true")
    ap.add_argument("--debug", type=int, default=0, metavar="N",
                    help="dfu-programmer style debug level (try 100 or 200)")
    ap.add_argument("--bus", type=int, default=0,
                    help="restrict to this USB bus number (with --address)")
    ap.add_argument("--address", type=int, default=0,
                    help="restrict to this USB device address")
    sub = ap.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("flash-hex", help="flash an Intel HEX + EEP pair directly")
    p.add_argument("hex_file")
    p.add_argument("eep_file")
    p.add_argument("--no-scramble", action="store_true",
                   help="do not apply the address keyed XOR (for stock Atmel "
                        "boot loaders that do not de-scramble)")
    p.add_argument("--suppress-validation", action="store_true",
                   help="skip the read-back step entirely")
    p.set_defaults(func=cmd_flash_hex)

    p = sub.add_parser("erase", help="chip erase")
    p.add_argument("--force", action="store_true", default=True)
    p.add_argument("--no-force", dest="force", action="store_false")
    p.set_defaults(func=cmd_erase)

    p = sub.add_parser("reset", help="start the application via watchdog reset")
    p.set_defaults(func=_simple(lambda d, t, a: atmel_start_app_reset(d)))

    p = sub.add_parser("launch", help="start the application without a reset")
    p.set_defaults(func=_simple(lambda d, t, a: atmel_start_app_noreset(d)))

    p = sub.add_parser("info", help="read the boot loader / device identifiers")
    p.set_defaults(func=_simple(_do_info))

    p = sub.add_parser("dump", help="read flash or eeprom back to a file")
    p.add_argument("out")
    p.add_argument("--eeprom", action="store_true")
    p.add_argument("--no-scramble", action="store_true",
                   help="keep the raw on-the-wire bytes")
    p.set_defaults(func=_simple(_do_dump))

    args = ap.parse_args(argv)
    try:
        return args.func(args) or 0
    except DfuError as exc:
        sys.stderr.write("error: %s\n" % exc)
        return 1


if __name__ == "__main__":
    sys.exit(main())

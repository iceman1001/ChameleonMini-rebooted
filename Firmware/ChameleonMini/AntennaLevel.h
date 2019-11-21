/* Copyright 2013 Timo Kasper, Simon Küppers, David Oswald ("ORIGINAL
 * AUTHORS"). All rights reserved.
 *
 * DEFINITIONS:
 *
 * "WORK": The material covered by this license includes the schematic
 * diagrams, designs, circuit or circuit board layouts, mechanical
 * drawings, documentation (in electronic or printed form), source code,
 * binary software, data files, assembled devices, and any additional
 * material provided by the ORIGINAL AUTHORS in the ChameleonMini project
 * (https://github.com/skuep/ChameleonMini).
 *
 * LICENSE TERMS:
 *
 * Redistributions and use of this WORK, with or without modification, or
 * of substantial portions of this WORK are permitted provided that the
 * following conditions are met:
 *
 * Redistributions and use of this WORK, with or without modification, or
 * of substantial portions of this WORK must include the above copyright
 * notice, this list of conditions, the below disclaimer, and the following
 * attribution:
 *
 * "Based on ChameleonMini an open-source RFID emulator:
 * https://github.com/skuep/ChameleonMini"
 *
 * The attribution must be clearly visible to a user, for example, by being
 * printed on the circuit board and an enclosure, and by being displayed by
 * software (both in binary and source code form).
 *
 * At any time, the majority of the ORIGINAL AUTHORS may decide to give
 * written permission to an entity to use or redistribute the WORK (with or
 * without modification) WITHOUT having to include the above copyright
 * notice, this list of conditions, the below disclaimer, and the above
 * attribution.
 *
 * DISCLAIMER:
 *
 * THIS PRODUCT IS PROVIDED BY THE ORIGINAL AUTHORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE ORIGINAL AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS PRODUCT, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the hardware, software, and
 * documentation should not be interpreted as representing official
 * policies, either expressed or implied, of the ORIGINAL AUTHORS.
 */

#ifndef ANTENNALEVEL_H_
#define ANTENNALEVEL_H_

#include "Common.h"

#define ANTENNA_LEVEL_R1            10E3
#define ANTENNA_LEVEL_R2            220E0
#define ANTENNA_LEVEL_VREF          1.0
#define ANTENNA_LEVEL_RES           4096
#define ANTENNA_LEVEL_OFFSET        190 /* LSB */

#define ANTENNA_LEVEL_MILLIVOLT     1E3
#define ANTENNA_LEVEL_FACTOR        (ANTENNA_LEVEL_VREF * (ANTENNA_LEVEL_R1 + ANTENNA_LEVEL_R2) / (ANTENNA_LEVEL_RES * ANTENNA_LEVEL_R2) )
#define ANTENNA_LEVEL_SCALE         ((uint32_t) 1<<16)
#define ANTENNA_LEVEL_NUMERATOR     ((uint32_t) (ANTENNA_LEVEL_MILLIVOLT * ANTENNA_LEVEL_FACTOR * ANTENNA_LEVEL_SCALE + .5))
#define ANTENNA_LEVEL_DENOMINATOR   (ANTENNA_LEVEL_SCALE)

#define ANTENNA_FIELD_MIN_RSSI      500

void AntennaLevelInit(void);
uint16_t AntennaLevelGet(void);
void AntennaLevelTick(void);

#endif /* ANTENNALEVEL_H_ */

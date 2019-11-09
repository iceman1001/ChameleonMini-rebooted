/*
 * Configuration.h
 *
 *  Created on: 15.02.2013
 *      Author: skuser
 */
#ifndef _CM_CONFIGURATION_H_
#define _CM_CONFIGURATION_H_

#include <stdint.h>
#include <stdbool.h>

#define CONFIGURATION_NAME_LENGTH_MAX   32
#define CONFIGURATION_UID_SIZE_MAX      16
#define CONFIGURATION_DUMMY_UID_SIZE    4
#define CONFIGURATION_DUMMY_UID_PART    0x00
#define CONFIGURATION_DUMMY_ATQA        0x0000
#define CONFIGURATION_DUMMY_SAK         0x00
#define CONFIGURATION_DUMMY_MEMSIZE     16

typedef uint8_t ConfigurationUidType[CONFIGURATION_UID_SIZE_MAX];

typedef enum  {
    /* This HAS to be the first element */
    CONFIG_NONE = 0,

#ifdef CONFIG_MF_ULTRALIGHT_SUPPORT
    CONFIG_MF_ULTRALIGHT,
    CONFIG_MF_ULTRALIGHT_EV1_80B,
    CONFIG_MF_ULTRALIGHT_EV1_164B,
#endif
#ifdef CONFIG_MF_CLASSIC_1K_SUPPORT
    CONFIG_MF_CLASSIC_1K,
#endif
#ifdef CONFIG_MF_CLASSIC_1K_7B_SUPPORT
    CONFIG_MF_CLASSIC_1K_7B,
#endif
#ifdef CONFIG_MF_CLASSIC_4K_SUPPORT
    CONFIG_MF_CLASSIC_4K,
#endif
#ifdef CONFIG_MF_CLASSIC_4K_7B_SUPPORT
    CONFIG_MF_CLASSIC_4K_7B,
#endif
#ifdef CONFIG_MF_CLASSIC_MINI_SUPPORT
    CONFIG_MF_CLASSIC_MINI,
#endif
#ifdef CONFIG_MF_DETECTION_SUPPORT
    CONFIG_MF_DETECTION,
#endif
#ifdef CONFIG_MF_CLASSIC_BRUTE_SUPPORT
    CONFIG_MF_CLASSIC_BRUTE,
#endif
    /* This HAS to be the last element */
    CONFIG_COUNT
} ConfigurationEnum;

/** With this `struct` the behavior of a configuration is defined. */
typedef struct {
    /**
     * \defgroup
     * \name Codec
     * Codec related methods.
     * @{
     */
    /** Function that initializes the codec. */
    void (*CodecInitFunc) (void);
    /** Function that deinitializes the codec. */
    void (*CodecDeInitFunc) (void);
    /** Function that is called on every iteration of the main loop.
     * Within this function the essential codec work is done.
     */
    void (*CodecTaskFunc) (void);
    /**
     * @}
     */

    /**
     * \defgroup
     * \name Application
     * Application related functions.
     * @{
     */
    /** Function that initializes the application. */
    void (*ApplicationInitFunc) (void);
    /** Function that resets the application. */
    void (*ApplicationResetFunc) (void);
    /** Function that is called on every iteration of the main loop. Application work that is independent from the codec layer can be done here. */
    void (*ApplicationTaskFunc) (void);
    /** Function that is called roughly every 100ms. This can be used for parallel tasks of the application, that is independent of the codec module. */
    void (*ApplicationTickFunc) (void);
    /** Function that is called when the "CARD_FUNCTION" button is pushed */
    void (*ApplicationButtonFunc) (void);
    /** This function does two important things. It gets called by the codec.
     *  The first task is to deliver data that have been received by the codec module to
     *  the application module. The application then can decide how to answer to these data and return
     *  the response to the codec module, which will process it according to the configured codec.
     *
     * \param ByteBuffer	Pointer to the start of the buffer, where the received data are and where the
     * 						application can put the response data.
     * \param BitCount		Number of bits that have been received.
     *
     * \return				Number of bits of the response.
     */
    uint16_t (*ApplicationProcessFunc) (uint8_t* ByteBuffer, uint16_t BitCount);
    /**
     * Writes the UID for the current configuration to the given buffer.
     * \param Uid	The target buffer.
     */
    void (*ApplicationGetUidFunc) (ConfigurationUidType Uid);
    /**
     * Writes a given UID to the current configuration.
     * \param Uid	The source buffer.
     */
    void (*ApplicationSetUidFunc) (ConfigurationUidType Uid);
    /**
     * Writes the SAK for the current configuration to the given buffer.
     * \param Sak	The target buffer.
     */
    void (*ApplicationGetSakFunc) (uint8_t * Sak);
    /**
     * Writes a given SAK to the current configuration.
     * \param Sak	The source buffer.
     */
    void (*ApplicationSetSakFunc) (uint8_t Sak);
    /**
     * Writes the ATQA for the current configuration to the given buffer.
     * \param Atqa	The target buffer.
     */
    void (*ApplicationGetAtqaFunc) (uint16_t * Atqa);
    /**
     * Writes a given ATQA to the current configuration.
     * \param Atqa	The source buffer.
     */
    void (*ApplicationSetAtqaFunc) (uint16_t Atqa);
    /**
     * @}
     */

    /**
     * Defines how many space the configuration needs. For emulating configurations this is the memory space of
     * the emulated card.
     */
    uint32_t MemorySize;
    /**
     * Defines the size of the UID for emulating configurations.
     */
    uint8_t UidSize;
    /**
     * Implies whether the Memory can be changed.
     */
    bool ReadOnly;

} ConfigurationType;

extern ConfigurationType ActiveConfiguration;

void ConfigurationInit(void);
void ConfigurationSetById(ConfigurationEnum Configuration);
void ConfigurationGetByName(char* Configuration, uint16_t BufferSize);
bool ConfigurationSetByName(const char* Configuration);
void ConfigurationGetList(char* ConfigurationList, uint16_t BufferSize);
uint32_t ConfigurationTableGetMemorySizeForId(ConfigurationEnum Configuration);

#endif /* _CM_CONFIGURATION_H_ */

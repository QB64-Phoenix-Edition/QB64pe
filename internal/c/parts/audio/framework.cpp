//----------------------------------------------------------------------------------------------------------------------
//    ___  ___   __ _ _  ___ ___     _          _ _       ___           _
//   / _ \| _ ) / /| | || _ \ __|   /_\ _  _ __| (_)___  | __|_ _  __ _(_)_ _  ___
//  | (_) | _ \/ _ \_  _|  _/ _|   / _ \ || / _` | / _ \ | _|| ' \/ _` | | ' \/ -_)
//   \__\_\___/\___/ |_||_| |___| /_/ \_\_,_\__,_|_\___/ |___|_||_\__, |_|_||_\___|
//                                                                |___/
//
//  QB64-PE Audio Engine powered by miniaudio (https://miniaud.io/)
//
//----------------------------------------------------------------------------------------------------------------------

#include "framework.h"

class AudioDecoderBackend {
  public:
    /// @brief This attaches the format decode VTables array to ma_resource_manager_config.
    /// @param maDecoderConfig Pointer to a miniaudio resource manager config object. This cannot be NULL.
    void Attach(ma_resource_manager_config *maResourceManagerConfig) {
        // Attach the VTable
        maResourceManagerConfig->ppCustomDecodingBackendVTables = decoders;
        maResourceManagerConfig->customDecodingBackendCount = MAX_DECODERS;
    }

    /// @brief Registers a custom decoding backend VTable and inserts it at the top of the array, so it has highest priority.
    /// @param vtable Pointer to a miniaudio decoding backend VTable. This cannot be NULL.
    /// @return true if the VTable was successfully registered, false otherwise.
    bool Register(ma_decoding_backend_vtable *vtable) {
        if (vtable && decoderCount < MAX_DECODERS) {
            if (decoderCount > 0) {
                for (size_t i = decoderCount; i > 0; --i) {
                    decoders[i] = decoders[i - 1];
                }
            }

            decoders[0] = vtable;
            ++decoderCount;

            return true;
        }

        return false;
    }

    /// @brief Returns the singleton instance of the AudioDecoderBackend.
    /// @return The singleton instance of the AudioDecoderBackend.
    static AudioDecoderBackend &Instance() {
        static AudioDecoderBackend instance;
        return instance;
    }

  private:
    static const ma_uint32 MAX_DECODERS = 16u;

    ma_decoding_backend_vtable dummyDecoder;
    ma_decoding_backend_vtable *decoders[MAX_DECODERS];
    ma_uint32 decoderCount;

    AudioDecoderBackend() {
        dummyDecoder = {};
        std::fill(decoders, decoders + MAX_DECODERS, &dummyDecoder);
        decoderCount = 0;
    }

    AudioDecoderBackend(const AudioDecoderBackend &) = delete;
    AudioDecoderBackend(AudioDecoderBackend &&) = delete;
    AudioDecoderBackend &operator=(const AudioDecoderBackend &) = delete;
    AudioDecoderBackend &operator=(AudioDecoderBackend &&) = delete;
};

/// @brief C-style wrapper for AudioDecoderBackend::Register.
/// @param vtable Pointer to a miniaudio decoding backend VTable.
/// @return true if the VTable was successfully registered, false otherwise.
bool AudioDecoderBackend_Register(ma_decoding_backend_vtable *vtable) {
    return AudioDecoderBackend::Instance().Register(vtable);
}

/// @brief C-style wrapper for AudioDecoderBackend::Attach.
/// @param maDecoderConfig Pointer to a miniaudio resource manager config object.
void AudioDecoderBackend_Attach(ma_resource_manager_config *maResourceManagerConfig) {
    AudioDecoderBackend::Instance().Attach(maResourceManagerConfig);
}

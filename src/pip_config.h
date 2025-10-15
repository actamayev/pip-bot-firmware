#pragma once
#include <cstring>  // Add this for strcmp

// PIP IDs for each environment
#define PIP_ID_LOCAL "9YhsJ"
#define PIP_ID_STAGING "bax2P"  // or "EZ2mS" as noted in your config
#define PIP_ID_PRODUCTION "PmKJZ"

// Returns the appropriate PIP ID based on the build environment
inline const char* getDefaultPipId() {
    #if defined(DEFAULT_ENVIRONMENT)
        const char* env = DEFAULT_ENVIRONMENT;
        
        if (strcmp(env, "staging") == 0) {
            return PIP_ID_STAGING;
        } else if (strcmp(env, "production") == 0) {
            return PIP_ID_PRODUCTION;
        }
    #endif
    
    // Default to local
    return PIP_ID_LOCAL;
}

// Keep this for backwards compatibility if needed elsewhere
#define DEFAULT_PIP_ID getDefaultPipId()

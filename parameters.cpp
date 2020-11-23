#include "parameters.h"


static const char GUIO_SIGNATURE[4] PROGMEM = { 'G', 'U', 'I', 'O' };


bool parameters_valid (const parameters_t *params)
{
    // Verify signature
    return memcmp_P(params->sig, GUIO_SIGNATURE, 4) == 0;
}

void parameters_init (parameters_t *const params)
{
    // Clear the structure
    memset(params, 0, sizeof(parameters_t));

    // Initialize header
    memcpy_P(params->sig, GUIO_SIGNATURE, 4);
    params->version = 1;
    //params->configured = false;
    //params->force_ap = false;
}

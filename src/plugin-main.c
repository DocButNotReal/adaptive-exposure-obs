#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("adaptive-exposure", "en-US")

extern struct obs_source_info adaptive_exposure_filter_info;

bool obs_module_load(void)
{
    obs_register_source(&adaptive_exposure_filter_info);
    blog(LOG_INFO, "[Adaptive Exposure] plugin loaded");
    return true;
}

const char *obs_module_description(void)
{
    return "Automatically brightens dark gameplay and removes the boost in well-lit scenes.";
}

#include <obs-module.h>
#include <graphics/effect.h>
#include <graphics/vec4.h>
#include <util/platform.h>
#include <math.h>
#include <stdint.h>

#define BLOG_PREFIX "[Adaptive Exposure] "

enum adaptive_state {
	ADAPTIVE_STATE_BRIGHT,
	ADAPTIVE_STATE_WAITING_DARK,
	ADAPTIVE_STATE_DARK,
	ADAPTIVE_STATE_WAITING_BRIGHT,
	ADAPTIVE_STATE_DISABLED,
	ADAPTIVE_STATE_IGNORED_BLACK,
};

struct adaptive_exposure_filter {
	obs_source_t *context;

	gs_effect_t *effect;
	gs_eparam_t *boost_param;

	gs_texrender_t *sample_render;
	gs_stagesurf_t *stage_surface;

	obs_hotkey_id toggle_hotkey;

	float dark_threshold;
	float light_threshold;
	float maximum_boost;

	float fade_in_seconds;
	float fade_out_seconds;
	float dark_delay_seconds;
	float bright_delay_seconds;

	float current_boost;
	float target_boost;

	float measured_luma;
	float smoothed_luma;
	float smoothing_strength;

	float sample_timer;
	float sample_interval;
	float dark_timer;
	float bright_timer;

	float exposure_curve;
	float black_ignore_threshold;
	float black_limited_boost;

	uint32_t sample_width;
	uint32_t sample_height;

	bool enabled;
	bool ignore_black;
	bool debug_logging;
	bool sample_ready;

	enum adaptive_state state;
};

static const char *state_name(enum adaptive_state state)
{
	switch (state) {
	case ADAPTIVE_STATE_BRIGHT:
		return "Bright";
	case ADAPTIVE_STATE_WAITING_DARK:
		return "Waiting for dark delay";
	case ADAPTIVE_STATE_DARK:
		return "Boost active";
	case ADAPTIVE_STATE_WAITING_BRIGHT:
		return "Waiting for bright delay";
	case ADAPTIVE_STATE_DISABLED:
		return "Disabled";
	case ADAPTIVE_STATE_IGNORED_BLACK:
		return "Near-black ignored";
	default:
		return "Unknown";
	}
}

static void set_state(struct adaptive_exposure_filter *filter,
		      enum adaptive_state state)
{
	if (filter->state == state)
		return;

	filter->state = state;

	if (filter->debug_logging) {
		blog(LOG_INFO,
		     BLOG_PREFIX "State: %s",
		     state_name(state));
	}
}

static const char *filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Filter.Name");
}

static void filter_update(void *data, obs_data_t *settings)
{
	struct adaptive_exposure_filter *filter = data;

	filter->enabled =
		obs_data_get_bool(settings, "enabled");

	filter->dark_threshold =
		(float)obs_data_get_double(settings, "dark_threshold") /
		255.0f;

	filter->light_threshold =
		(float)obs_data_get_double(settings, "light_threshold") /
		255.0f;

	filter->maximum_boost =
		(float)obs_data_get_double(settings, "maximum_boost");

	filter->fade_in_seconds =
		(float)obs_data_get_double(settings, "fade_in_seconds");

	filter->fade_out_seconds =
		(float)obs_data_get_double(settings, "fade_out_seconds");

	filter->dark_delay_seconds =
		(float)obs_data_get_double(settings, "dark_delay_seconds");

	filter->bright_delay_seconds =
		(float)obs_data_get_double(settings, "bright_delay_seconds");

	filter->smoothing_strength =
		(float)obs_data_get_double(settings, "smoothing_strength");

	filter->sample_interval =
		(float)obs_data_get_double(settings, "sample_interval_ms") /
		1000.0f;

	filter->exposure_curve =
		(float)obs_data_get_double(settings, "exposure_curve");

	filter->black_ignore_threshold =
		(float)obs_data_get_double(settings, "black_ignore_threshold") /
		255.0f;

	filter->black_limited_boost =
		(float)obs_data_get_double(settings, "black_limited_boost");

	filter->ignore_black =
		obs_data_get_bool(settings, "ignore_black");

	filter->debug_logging =
		obs_data_get_bool(settings, "debug_logging");

	if (filter->light_threshold <= filter->dark_threshold) {
		filter->light_threshold =
			filter->dark_threshold + (1.0f / 255.0f);
	}

	if (filter->fade_in_seconds < 0.05f)
		filter->fade_in_seconds = 0.05f;

	if (filter->fade_out_seconds < 0.05f)
		filter->fade_out_seconds = 0.05f;

	if (filter->sample_interval < 0.10f)
		filter->sample_interval = 0.10f;

	if (filter->smoothing_strength < 0.0f)
		filter->smoothing_strength = 0.0f;

	if (filter->smoothing_strength > 0.95f)
		filter->smoothing_strength = 0.95f;

	if (filter->exposure_curve < 0.25f)
		filter->exposure_curve = 0.25f;

	if (filter->exposure_curve > 4.0f)
		filter->exposure_curve = 4.0f;

	if (filter->black_limited_boost < 0.0f)
		filter->black_limited_boost = 0.0f;

	if (filter->black_limited_boost > filter->maximum_boost)
		filter->black_limited_boost = filter->maximum_boost;

	if (!filter->enabled) {
		filter->dark_timer = 0.0f;
		filter->bright_timer = 0.0f;
		filter->target_boost = 0.0f;
		set_state(filter, ADAPTIVE_STATE_DISABLED);
	}
}

static void filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(
		settings, "enabled", true);

	obs_data_set_default_double(
		settings, "dark_threshold", 45.0);

	obs_data_set_default_double(
		settings, "light_threshold", 62.0);

	obs_data_set_default_double(
		settings, "maximum_boost", 0.45);

	obs_data_set_default_double(
		settings, "fade_in_seconds", 1.5);

	obs_data_set_default_double(
		settings, "fade_out_seconds", 0.8);

	obs_data_set_default_double(
		settings, "dark_delay_seconds", 1.0);

	obs_data_set_default_double(
		settings, "bright_delay_seconds", 0.5);

	obs_data_set_default_double(
		settings, "smoothing_strength", 0.65);

	obs_data_set_default_double(
		settings, "sample_interval_ms", 250.0);

	obs_data_set_default_double(
		settings, "exposure_curve", 1.6);

	obs_data_set_default_double(
		settings, "black_ignore_threshold", 3.0);

	obs_data_set_default_double(
		settings, "black_limited_boost", 0.10);

	obs_data_set_default_bool(
		settings, "ignore_black", true);

	obs_data_set_default_bool(
		settings, "debug_logging", false);
}

static obs_properties_t *filter_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props =
		obs_properties_create();

	obs_properties_add_bool(
		props,
		"enabled",
		obs_module_text("Enabled"));

	obs_properties_add_float_slider(
		props,
		"dark_threshold",
		obs_module_text("DarkThreshold"),
		0.0,
		255.0,
		1.0);

	obs_properties_add_float_slider(
		props,
		"light_threshold",
		obs_module_text("LightThreshold"),
		0.0,
		255.0,
		1.0);

	obs_properties_add_float_slider(
		props,
		"maximum_boost",
		obs_module_text("MaximumBoost"),
		0.0,
		1.5,
		0.01);

	obs_properties_add_float_slider(
		props,
		"exposure_curve",
		obs_module_text("ExposureCurve"),
		0.25,
		4.0,
		0.05);

	obs_properties_add_float_slider(
		props,
		"fade_in_seconds",
		obs_module_text("FadeInSeconds"),
		0.05,
		10.0,
		0.05);

	obs_properties_add_float_slider(
		props,
		"fade_out_seconds",
		obs_module_text("FadeOutSeconds"),
		0.05,
		10.0,
		0.05);

	obs_properties_add_float_slider(
		props,
		"dark_delay_seconds",
		obs_module_text("DarkDelaySeconds"),
		0.0,
		10.0,
		0.1);

	obs_properties_add_float_slider(
		props,
		"bright_delay_seconds",
		obs_module_text("BrightDelaySeconds"),
		0.0,
		10.0,
		0.1);

	obs_properties_add_float_slider(
		props,
		"smoothing_strength",
		obs_module_text("SmoothingStrength"),
		0.0,
		0.95,
		0.05);

	obs_properties_add_float_slider(
		props,
		"sample_interval_ms",
		obs_module_text("SampleInterval"),
		100.0,
		2000.0,
		50.0);

	obs_properties_add_bool(
		props,
		"ignore_black",
		obs_module_text("IgnoreBlack"));

	obs_properties_add_float_slider(
		props,
		"black_ignore_threshold",
		obs_module_text("BlackIgnoreThreshold"),
		0.0,
		25.0,
		0.5);

	obs_properties_add_float_slider(
		props,
		"black_limited_boost",
		obs_module_text("BlackLimitedBoost"),
		0.0,
		0.5,
		0.01);

	obs_properties_add_bool(
		props,
		"debug_logging",
		obs_module_text("DebugLogging"));

	return props;
}

static void toggle_hotkey_callback(void *data,
				   obs_hotkey_id id,
				   obs_hotkey_t *hotkey,
				   bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	if (!pressed)
		return;

	struct adaptive_exposure_filter *filter = data;

	if (!filter || !filter->context)
		return;

	obs_data_t *settings =
		obs_source_get_settings(filter->context);

	if (!settings)
		return;

	const bool new_enabled = !filter->enabled;

	obs_data_set_bool(
		settings,
		"enabled",
		new_enabled);

	obs_source_update(
		filter->context,
		settings);

	obs_data_release(settings);

	blog(LOG_INFO,
	     BLOG_PREFIX "Automation %s via hotkey",
	     new_enabled ? "enabled" : "disabled");
}

static void *filter_create(obs_data_t *settings,
			   obs_source_t *context)
{
	struct adaptive_exposure_filter *filter =
		bzalloc(sizeof(struct adaptive_exposure_filter));

	if (!filter)
		return NULL;

	filter->context = context;
	filter->toggle_hotkey = OBS_INVALID_HOTKEY_ID;

	filter->measured_luma = 1.0f;
	filter->smoothed_luma = 1.0f;

	filter->sample_width = 64;
	filter->sample_height = 36;
	filter->sample_timer = 0.0f;
	filter->sample_ready = false;

	filter->state = ADAPTIVE_STATE_BRIGHT;

	char *effect_path =
		obs_module_file(
			"effects/adaptive-exposure.effect");

	char *error_string = NULL;

	obs_enter_graphics();

	filter->effect =
		gs_effect_create_from_file(
			effect_path,
			&error_string);

	filter->sample_render =
		gs_texrender_create(
			GS_BGRA,
			GS_ZS_NONE);

	filter->stage_surface =
		gs_stagesurface_create(
			filter->sample_width,
			filter->sample_height,
			GS_BGRA);

	obs_leave_graphics();

	bfree(effect_path);

	if (!filter->effect) {
		blog(LOG_ERROR,
		     BLOG_PREFIX "Could not load shader: %s",
		     error_string
			     ? error_string
			     : "unknown error");
		goto fail;
	}

	filter->boost_param =
		gs_effect_get_param_by_name(
			filter->effect,
			"boost");

	if (!filter->boost_param) {
		blog(LOG_ERROR,
		     BLOG_PREFIX
		     "Shader parameter 'boost' was not found");
		goto fail;
	}

	if (!filter->sample_render ||
	    !filter->stage_surface) {
		blog(LOG_ERROR,
		     BLOG_PREFIX
		     "Could not create luminance sampling resources");
		goto fail;
	}

	filter_update(filter, settings);

	filter->toggle_hotkey =
		obs_hotkey_register_source(
			context,
			"adaptive_exposure.toggle",
			obs_module_text("Hotkey.Toggle"),
			toggle_hotkey_callback,
			filter);

	bfree(error_string);

	blog(LOG_INFO,
	     BLOG_PREFIX "GPU luminance sampler created");

	return filter;

fail:
	bfree(error_string);

	obs_enter_graphics();

	if (filter->sample_render)
		gs_texrender_destroy(filter->sample_render);

	if (filter->stage_surface)
		gs_stagesurface_destroy(filter->stage_surface);

	if (filter->effect)
		gs_effect_destroy(filter->effect);

	obs_leave_graphics();

	bfree(filter);
	return NULL;
}

static void filter_destroy(void *data)
{
	struct adaptive_exposure_filter *filter = data;

	if (!filter)
		return;

	if (filter->toggle_hotkey != OBS_INVALID_HOTKEY_ID) {
		obs_hotkey_unregister(
			filter->toggle_hotkey);
	}

	obs_enter_graphics();

	if (filter->sample_render)
		gs_texrender_destroy(
			filter->sample_render);

	if (filter->stage_surface)
		gs_stagesurface_destroy(
			filter->stage_surface);

	if (filter->effect)
		gs_effect_destroy(
			filter->effect);

	obs_leave_graphics();

	bfree(filter);
}

static float centre_weight(uint32_t x,
			   uint32_t y,
			   uint32_t width,
			   uint32_t height)
{
	const float nx =
		((float)x + 0.5f) / (float)width;

	const float ny =
		((float)y + 0.5f) / (float)height;

	const float dx = nx - 0.5f;
	const float dy = ny - 0.5f;

	const float distance_squared =
		(dx * dx) + (dy * dy);

	/*
	 * Centre pixels receive more influence, while the
	 * edges still contribute enough to avoid tunnel vision.
	 */
	float weight =
		1.0f - (distance_squared * 2.4f);

	if (weight < 0.20f)
		weight = 0.20f;

	return weight;
}

static bool read_staged_luminance(
	struct adaptive_exposure_filter *filter)
{
	uint8_t *data = NULL;
	uint32_t linesize = 0;

	if (!gs_stagesurface_map(
		    filter->stage_surface,
		    &data,
		    &linesize)) {
		return false;
	}

	double weighted_luminance_sum = 0.0;
	double total_weight = 0.0;

	for (uint32_t y = 0;
	     y < filter->sample_height;
	     y++) {
		const uint8_t *row =
			data + ((size_t)y * linesize);

		for (uint32_t x = 0;
		     x < filter->sample_width;
		     x++) {
			const uint8_t *pixel =
				row + ((size_t)x * 4);

			const float blue =
				(float)pixel[0];

			const float green =
				(float)pixel[1];

			const float red =
				(float)pixel[2];

			const float luma =
				(0.2126f * red) +
				(0.7152f * green) +
				(0.0722f * blue);

			const float weight =
				centre_weight(
					x,
					y,
					filter->sample_width,
					filter->sample_height);

			weighted_luminance_sum +=
				(double)luma *
				(double)weight;

			total_weight +=
				(double)weight;
		}
	}

	gs_stagesurface_unmap(
		filter->stage_surface);

	if (total_weight <= 0.0)
		return false;

	const float average_luma_255 =
		(float)(
			weighted_luminance_sum /
			total_weight);

	filter->measured_luma =
		average_luma_255 / 255.0f;

	const float keep_old =
		filter->smoothing_strength;

	const float use_new =
		1.0f - keep_old;

	filter->smoothed_luma =
		(filter->smoothed_luma * keep_old) +
		(filter->measured_luma * use_new);

	if (filter->debug_logging) {
		blog(LOG_INFO,
		     BLOG_PREFIX
		     "Luminance raw=%.1f smoothed=%.1f "
		     "boost=%.3f target=%.3f state=%s",
		     average_luma_255,
		     filter->smoothed_luma * 255.0f,
		     filter->current_boost,
		     filter->target_boost,
		     state_name(filter->state));
	}

	return true;
}

static bool stage_source_sample(
	struct adaptive_exposure_filter *filter)
{
	obs_source_t *target =
		obs_filter_get_target(
			filter->context);

	if (!target)
		return false;

	const uint32_t source_width =
		obs_source_get_width(target);

	const uint32_t source_height =
		obs_source_get_height(target);

	if (source_width == 0 ||
	    source_height == 0) {
		return false;
	}

	gs_texrender_reset(
		filter->sample_render);

	gs_viewport_push();
	gs_projection_push();
	gs_matrix_push();
	gs_blend_state_push();

	bool rendered = false;

	if (gs_texrender_begin(
		    filter->sample_render,
		    filter->sample_width,
		    filter->sample_height)) {
		struct vec4 clear_color = {
			0.0f,
			0.0f,
			0.0f,
			0.0f
		};

		gs_clear(
			GS_CLEAR_COLOR,
			&clear_color,
			0.0f,
			0);

		gs_ortho(
			0.0f,
			(float)source_width,
			0.0f,
			(float)source_height,
			-100.0f,
			100.0f);

		gs_matrix_identity();
		gs_reset_blend_state();

		obs_source_video_render(target);

		gs_texrender_end(
			filter->sample_render);

		rendered = true;
	}

	gs_blend_state_pop();
	gs_matrix_pop();
	gs_projection_pop();
	gs_viewport_pop();

	if (!rendered)
		return false;

	gs_texture_t *sample_texture =
		gs_texrender_get_texture(
			filter->sample_render);

	if (!sample_texture)
		return false;

	gs_stage_texture(
		filter->stage_surface,
		sample_texture);

	return true;
}

static float calculate_dark_boost(
	struct adaptive_exposure_filter *filter,
	float luma)
{
	const float range =
		filter->dark_threshold;

	float darkness =
		range > 0.0001f
			? (filter->dark_threshold - luma) /
				  range
			: 1.0f;

	if (darkness < 0.0f)
		darkness = 0.0f;

	if (darkness > 1.0f)
		darkness = 1.0f;

	/*
	 * Nonlinear curve: low darkness gets a gentle lift,
	 * while genuinely dark scenes receive more help.
	 */
	const float curved_darkness =
		powf(darkness, filter->exposure_curve);

	return curved_darkness *
	       filter->maximum_boost;
}

static void update_adaptive_target(
	struct adaptive_exposure_filter *filter,
	float seconds)
{
	if (!filter->enabled) {
		filter->dark_timer = 0.0f;
		filter->bright_timer = 0.0f;
		filter->target_boost = 0.0f;
		set_state(filter, ADAPTIVE_STATE_DISABLED);
		return;
	}

	const float luma =
		filter->smoothed_luma;

	const bool near_black =
		luma < filter->black_ignore_threshold;

	if (filter->ignore_black && near_black) {
		filter->dark_timer = 0.0f;
		filter->bright_timer = 0.0f;

		/*
		 * A tiny limited boost is allowed so genuine very-dark
		 * gameplay is not treated exactly like a missing source.
		 */
		filter->target_boost =
			filter->black_limited_boost;

		set_state(
			filter,
			ADAPTIVE_STATE_IGNORED_BLACK);

		return;
	}

	if (luma < filter->dark_threshold) {
		filter->bright_timer = 0.0f;
		filter->dark_timer += seconds;

		if (filter->dark_timer <
		    filter->dark_delay_seconds) {
			set_state(
				filter,
				ADAPTIVE_STATE_WAITING_DARK);
			return;
		}

		filter->target_boost =
			calculate_dark_boost(
				filter,
				luma);

		set_state(
			filter,
			ADAPTIVE_STATE_DARK);

		return;
	}

	if (luma > filter->light_threshold) {
		filter->dark_timer = 0.0f;
		filter->bright_timer += seconds;

		if (filter->bright_timer <
		    filter->bright_delay_seconds) {
			set_state(
				filter,
				ADAPTIVE_STATE_WAITING_BRIGHT);
			return;
		}

		filter->target_boost = 0.0f;

		set_state(
			filter,
			ADAPTIVE_STATE_BRIGHT);

		return;
	}

	/*
	 * Inside the hysteresis band, keep the existing target
	 * and clear both activation timers.
	 */
	filter->dark_timer = 0.0f;
	filter->bright_timer = 0.0f;
}

static void update_boost_fade(
	struct adaptive_exposure_filter *filter,
	float seconds)
{
	const bool increasing =
		filter->target_boost >
		filter->current_boost;

	const float duration =
		increasing
			? filter->fade_in_seconds
			: filter->fade_out_seconds;

	float amount =
		seconds /
		(duration < 0.05f
			 ? 0.05f
			 : duration);

	if (amount > 1.0f)
		amount = 1.0f;

	filter->current_boost +=
		(filter->target_boost -
		 filter->current_boost) *
		amount;

	if (fabsf(
		    filter->current_boost -
		    filter->target_boost) <
	    0.0001f) {
		filter->current_boost =
			filter->target_boost;
	}
}

static void filter_tick(void *data, float seconds)
{
	struct adaptive_exposure_filter *filter =
		data;

	filter->sample_timer += seconds;

	update_adaptive_target(
		filter,
		seconds);

	update_boost_fade(
		filter,
		seconds);
}

static void filter_render(
	void *data,
	gs_effect_t *unused_effect)
{
	UNUSED_PARAMETER(unused_effect);

	struct adaptive_exposure_filter *filter =
		data;

	if (filter->sample_timer >=
	    filter->sample_interval) {
		filter->sample_timer = 0.0f;

		if (filter->sample_ready) {
			read_staged_luminance(
				filter);
		}

		if (stage_source_sample(filter)) {
			filter->sample_ready = true;
		}
	}

	if (!obs_source_process_filter_begin(
		    filter->context,
		    GS_RGBA,
		    OBS_ALLOW_DIRECT_RENDERING)) {
		return;
	}

	gs_effect_set_float(
		filter->boost_param,
		filter->current_boost);

	obs_source_process_filter_end(
		filter->context,
		filter->effect,
		0,
		0);
}

struct obs_source_info
	adaptive_exposure_filter_info = {
		.id = "adaptive_exposure_filter",
		.type = OBS_SOURCE_TYPE_FILTER,
		.output_flags = OBS_SOURCE_VIDEO,

		.get_name = filter_get_name,
		.create = filter_create,
		.destroy = filter_destroy,
		.update = filter_update,
		.get_defaults = filter_defaults,
		.get_properties = filter_properties,

		.video_tick = filter_tick,
		.video_render = filter_render,
};

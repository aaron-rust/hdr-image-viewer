#include "color_management.h"

#include <QPointF>
#include <QString>
#include <qpa/qplatformwindow_p.h>

#include <iomanip>
#include <sstream>
#include <string_view>

namespace {
    constexpr double PRIMARIES_SCALE = 1'000'000.0;
    constexpr double LUMINANCE_SCALE = 10'000.0;
}

ColorManagementGlobal::ColorManagementGlobal()
    : QWaylandClientExtensionTemplate<ColorManagementGlobal>(1)
{
    initialize();
    connect(this, &ColorManagementGlobal::activeChanged, this, [this]() {
        if (!isActive()) {
            wp_color_manager_v1_destroy(object());
        }
    });
}

void ColorManagementGlobal::wp_color_manager_v1_supported_feature(uint32_t /*feature*/)
{
    // Note: Should check supported features before using them
}

ImageDescriptionInfo::ImageDescriptionInfo(::wp_image_description_info_v1 *info)
    : QtWayland::wp_image_description_info_v1(info)
{
}

ImageDescriptionInfo::~ImageDescriptionInfo()
{
    wp_image_description_info_v1_destroy(object());
}

void ImageDescriptionInfo::wp_image_description_info_v1_done()
{
    buildDescription();
    Q_EMIT descriptionReady();
}

void ImageDescriptionInfo::wp_image_description_info_v1_primaries(int32_t r_x, int32_t r_y, 
                                                                 int32_t g_x, int32_t g_y, 
                                                                 int32_t b_x, int32_t b_y, 
                                                                 int32_t w_x, int32_t w_y)
{
    m_containerRed = QPointF(r_x, r_y) / PRIMARIES_SCALE;
    m_containerGreen = QPointF(g_x, g_y) / PRIMARIES_SCALE;
    m_containerBlue = QPointF(b_x, b_y) / PRIMARIES_SCALE;
    m_containerWhite = QPointF(w_x, w_y) / PRIMARIES_SCALE;
}

void ImageDescriptionInfo::wp_image_description_info_v1_primaries_named(uint32_t /*primaries*/)
{
    // Named primaries handled automatically
}

void ImageDescriptionInfo::wp_image_description_info_v1_tf_power(uint32_t /*eexp*/)
{
    // Power function handling if needed
}

void ImageDescriptionInfo::wp_image_description_info_v1_tf_named(uint32_t tf)
{
    m_transferFunction = tf;
}

void ImageDescriptionInfo::wp_image_description_info_v1_luminances(uint32_t min_lum, 
                                                                  uint32_t max_lum, 
                                                                  uint32_t reference_lum)
{
    m_minLuminance = min_lum / LUMINANCE_SCALE;
    m_maxLuminance = max_lum;
    m_referenceLuminance = reference_lum;
}

void ImageDescriptionInfo::wp_image_description_info_v1_target_primaries(int32_t r_x, int32_t r_y, 
                                                                        int32_t g_x, int32_t g_y, 
                                                                        int32_t b_x, int32_t b_y, 
                                                                        int32_t w_x, int32_t w_y)
{
    m_targetRed = QPointF(r_x, r_y) / PRIMARIES_SCALE;
    m_targetGreen = QPointF(g_x, g_y) / PRIMARIES_SCALE;
    m_targetBlue = QPointF(b_x, b_y) / PRIMARIES_SCALE;
    m_targetWhite = QPointF(w_x, w_y) / PRIMARIES_SCALE;
}

void ImageDescriptionInfo::wp_image_description_info_v1_target_luminance(uint32_t min_lum, uint32_t max_lum)
{
    m_targetMinLuminance = min_lum / LUMINANCE_SCALE;
    m_targetMaxLuminance = max_lum;
}

void ImageDescriptionInfo::wp_image_description_info_v1_target_max_cll(uint32_t /*max_cll*/)
{
    // Content light level handling if needed
}

void ImageDescriptionInfo::wp_image_description_info_v1_target_max_fall(uint32_t /*max_fall*/)
{
    // Frame average light level handling if needed
}

void ImageDescriptionInfo::buildDescription()
{
    std::string_view transferFunctionName;
    switch (m_transferFunction) {
    case WP_COLOR_MANAGER_V1_TRANSFER_FUNCTION_GAMMA22:
        transferFunctionName = "gamma 2.2 (sRGB)";
        break;
    case WP_COLOR_MANAGER_V1_TRANSFER_FUNCTION_ST2084_PQ:
        transferFunctionName = "PQ (HDR10)";
        break;
    default:
        transferFunctionName = "unknown";
        break;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3);
    oss << "\nColor Primaries:\n";
    oss << "  Red:   " << m_containerRed.x() << ", " << m_containerRed.y() << "\n";
    oss << "  Green: " << m_containerGreen.x() << ", " << m_containerGreen.y() << "\n";
    oss << "  Blue:  " << m_containerBlue.x() << ", " << m_containerBlue.y() << "\n";
    oss << "  White: " << m_containerWhite.x() << ", " << m_containerWhite.y() << "\n";
    
    oss << std::setprecision(2);
    oss << "Transfer Function: " << transferFunctionName << "\n";
    oss << "Luminance Range: [" << m_minLuminance << ", " << m_maxLuminance << "] nits\n";
    oss << "Reference Luminance: " << m_referenceLuminance << " nits\n";
    oss << "Target Range: [" << m_targetMinLuminance << ", " << m_targetMaxLuminance << "] nits";

    m_description = QString::fromStdString(oss.str());
}

CompositorImageDescription::CompositorImageDescription(::wp_image_description_v1 *descr)
    : QtWayland::wp_image_description_v1(descr)
    , m_info(wp_image_description_v1_get_information(descr))
{
    connect(&m_info, &ImageDescriptionInfo::descriptionReady, this, &CompositorImageDescription::ready);
}

CompositorImageDescription::~CompositorImageDescription()
{
    wp_image_description_v1_destroy(object());
}

PendingImageDescription::PendingImageDescription(QQuickWindow *window,
                                               ColorManagementSurface *surface,
                                               ::wp_image_description_v1 *descr,
                                               uint32_t renderIntent)
    : QtWayland::wp_image_description_v1(descr)
    , m_window(window)
    , m_surface(surface)
    , m_renderIntent(renderIntent)
{
}

PendingImageDescription::~PendingImageDescription()
{
    wp_image_description_v1_destroy(object());
}

void PendingImageDescription::wp_image_description_v1_ready(uint32_t /*identity*/)
{
    if (m_window && m_surface) {
        wp_color_management_surface_v1_set_image_description(m_surface->object(), object(), m_renderIntent);
        m_window->requestUpdate();
    }
    delete this;
}

ColorManagementFeedback::ColorManagementFeedback(::wp_color_management_surface_feedback_v1 *obj)
    : QtWayland::wp_color_management_surface_feedback_v1(obj)
    , m_preferred(std::make_unique<CompositorImageDescription>(get_preferred()))
{
    connect(m_preferred.get(), &CompositorImageDescription::ready, this, &ColorManagementFeedback::preferredChanged);
}

ColorManagementFeedback::~ColorManagementFeedback()
{
    wp_color_management_surface_feedback_v1_destroy(object());
}

void ColorManagementFeedback::wp_color_management_surface_feedback_v1_preferred_changed(uint32_t /*identity*/)
{
    if (m_preferred) {
        auto newPreferred = std::make_unique<CompositorImageDescription>(get_preferred());
        connect(newPreferred.get(), &CompositorImageDescription::ready, this, &ColorManagementFeedback::processPendingDescriptions);
        connect(newPreferred.get(), &CompositorImageDescription::ready, this, &ColorManagementFeedback::preferredChanged);
        m_pending.push_back(std::move(newPreferred));
    } else {
        m_preferred = std::make_unique<CompositorImageDescription>(get_preferred());
        connect(m_preferred.get(), &CompositorImageDescription::ready, this, &ColorManagementFeedback::preferredChanged);
    }
}

void ColorManagementFeedback::processPendingDescriptions()
{
    if (!m_pending.empty()) {
        m_preferred = std::move(m_pending.front());
        m_pending.pop_front();
    }
}

ColorManagementSurface::ColorManagementSurface(ColorManagementGlobal *global,
                                              QQuickWindow *window,
                                              ::wp_color_management_surface_v1 *obj,
                                              std::unique_ptr<ColorManagementFeedback> feedback)
    : QtWayland::wp_color_management_surface_v1(obj)
    , m_global(global)
    , m_window(window)
    , m_feedback(std::move(feedback))
{
}

ColorManagementSurface::~ColorManagementSurface()
{
    wp_color_management_surface_v1_destroy(object());
}

void ColorManagementSurface::setColorMode(ColorMode mode)
{
    if (mode == ColorMode::Default) {
        unset_image_description();
        m_window->requestUpdate();
        return;
    }

    createParametricDescription(mode);
}

void ColorManagementSurface::setPQMode(int referenceLuminance)
{
    createPQDescription(referenceLuminance);
}

void ColorManagementSurface::createParametricDescription(ColorMode mode)
{
    auto creator = m_global->create_parametric_creator();

    switch (mode) {
    case ColorMode::sRGB_Gamma22:
        wp_image_description_creator_params_v1_set_primaries_named(creator, QtWayland::wp_color_manager_v1::primaries_srgb);
        wp_image_description_creator_params_v1_set_tf_named(creator, QtWayland::wp_color_manager_v1::transfer_function_gamma22);
        wp_image_description_creator_params_v1_set_luminances(creator, 0, 200, 100);
        wp_image_description_creator_params_v1_set_mastering_luminance(creator, 0, 200);
        break;

    case ColorMode::BT2020_Gamma22:
        wp_image_description_creator_params_v1_set_primaries_named(creator, QtWayland::wp_color_manager_v1::primaries_bt2020);
        wp_image_description_creator_params_v1_set_tf_named(creator, QtWayland::wp_color_manager_v1::transfer_function_gamma22);
        break;

    case ColorMode::BT2020_PQ:
        wp_image_description_creator_params_v1_set_primaries_named(creator, QtWayland::wp_color_manager_v1::primaries_bt2020);
        wp_image_description_creator_params_v1_set_tf_named(creator, QtWayland::wp_color_manager_v1::transfer_function_st2084_pq);
        break;

    case ColorMode::PAL_M:
        wp_image_description_creator_params_v1_set_primaries_named(creator, QtWayland::wp_color_manager_v1::primaries_pal_m);
        wp_image_description_creator_params_v1_set_tf_named(creator, QtWayland::wp_color_manager_v1::transfer_function_gamma22);
        break;

    case ColorMode::CIE1931_XYZ:
        wp_image_description_creator_params_v1_set_primaries_named(creator, QtWayland::wp_color_manager_v1::primaries_cie1931_xyz);
        wp_image_description_creator_params_v1_set_tf_named(creator, QtWayland::wp_color_manager_v1::transfer_function_gamma22);
        break;

    default:
        return;
    }

    new PendingImageDescription(m_window, this, wp_image_description_creator_params_v1_create(creator));
}

void ColorManagementSurface::createPQDescription(int referenceLuminance)
{
    auto creator = m_global->create_parametric_creator();
    wp_image_description_creator_params_v1_set_primaries_named(creator, QtWayland::wp_color_manager_v1::primaries_bt2020);
    wp_image_description_creator_params_v1_set_tf_named(creator, QtWayland::wp_color_manager_v1::transfer_function_st2084_pq);
    wp_image_description_creator_params_v1_set_luminances(creator, 0, 10'000, referenceLuminance);
    wp_image_description_creator_params_v1_set_mastering_luminance(creator, 0, 1'000);
    
    new PendingImageDescription(m_window, this, wp_image_description_creator_params_v1_create(creator));
}

#include "moc_color_management.cpp"

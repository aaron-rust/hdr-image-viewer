#pragma once

#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QQuickWindow>
#include <QWaylandClientExtension>

#include <deque>
#include <memory>
#include <optional>
#include <unordered_map>

#include <qpa/qplatformwindow_p.h>
#include <qwayland-color-management-v1.h>

class ColorManagementSurface;

class ColorManagementGlobal : public QWaylandClientExtensionTemplate<ColorManagementGlobal>, 
                              public QtWayland::wp_color_manager_v1
{
    Q_OBJECT

public:
    explicit ColorManagementGlobal();
    ~ColorManagementGlobal() override = default;

protected:
    void wp_color_manager_v1_supported_feature(uint32_t feature) override;
};

class ImageDescriptionInfo : public QObject, 
                            public QtWayland::wp_image_description_info_v1
{
    Q_OBJECT

public:
    explicit ImageDescriptionInfo(::wp_image_description_info_v1 *info);
    ~ImageDescriptionInfo() override;

    const QString& description() const { return m_description; }

Q_SIGNALS:
    void descriptionReady();

protected:
    void wp_image_description_info_v1_done() override;
    void wp_image_description_info_v1_primaries(int32_t r_x, int32_t r_y, int32_t g_x, int32_t g_y, 
                                               int32_t b_x, int32_t b_y, int32_t w_x, int32_t w_y) override;
    void wp_image_description_info_v1_primaries_named(uint32_t primaries) override;
    void wp_image_description_info_v1_tf_power(uint32_t eexp) override;
    void wp_image_description_info_v1_tf_named(uint32_t tf) override;
    void wp_image_description_info_v1_luminances(uint32_t min_lum, uint32_t max_lum, uint32_t reference_lum) override;
    void wp_image_description_info_v1_target_primaries(int32_t r_x, int32_t r_y, int32_t g_x, int32_t g_y, 
                                                      int32_t b_x, int32_t b_y, int32_t w_x, int32_t w_y) override;
    void wp_image_description_info_v1_target_luminance(uint32_t min_lum, uint32_t max_lum) override;
    void wp_image_description_info_v1_target_max_cll(uint32_t max_cll) override;
    void wp_image_description_info_v1_target_max_fall(uint32_t max_fall) override;

private:
    void buildDescription();

    QString m_description;
    uint32_t m_transferFunction = 0;
    
    // Color primaries
    QPointF m_containerRed, m_containerGreen, m_containerBlue, m_containerWhite;
    QPointF m_targetRed, m_targetGreen, m_targetBlue, m_targetWhite;
    
    // Luminance values
    double m_minLuminance = 0.0;
    double m_maxLuminance = 0.0;
    double m_referenceLuminance = 0.0;
    double m_targetMinLuminance = 0.0;
    double m_targetMaxLuminance = 0.0;
};

class CompositorImageDescription : public QObject, 
                                  public QtWayland::wp_image_description_v1
{
    Q_OBJECT

public:
    explicit CompositorImageDescription(::wp_image_description_v1 *descr);
    ~CompositorImageDescription() override;

    ImageDescriptionInfo& info() { return m_info; }
    const ImageDescriptionInfo& info() const { return m_info; }

Q_SIGNALS:
    void ready();

private:
    ImageDescriptionInfo m_info;
};

class PendingImageDescription : public QtWayland::wp_image_description_v1
{
public:
    explicit PendingImageDescription(QQuickWindow *window,
                                   ColorManagementSurface *surface,
                                   ::wp_image_description_v1 *descr,
                                   uint32_t renderIntent = WP_COLOR_MANAGER_V1_RENDER_INTENT_PERCEPTUAL);
    ~PendingImageDescription() override;

protected:
    void wp_image_description_v1_ready(uint32_t identity) override;

private:
    QPointer<QQuickWindow> m_window;
    QPointer<ColorManagementSurface> m_surface;
    uint32_t m_renderIntent;
};

class ColorManagementFeedback : public QObject, 
                               public QtWayland::wp_color_management_surface_feedback_v1
{
    Q_OBJECT

public:
    explicit ColorManagementFeedback(::wp_color_management_surface_feedback_v1 *obj);
    ~ColorManagementFeedback() override;

    const std::unique_ptr<CompositorImageDescription>& preferred() const { return m_preferred; }

Q_SIGNALS:
    void preferredChanged();

protected:
    void wp_color_management_surface_feedback_v1_preferred_changed(uint32_t identity) override;

private:
    void processPendingDescriptions();

    std::unique_ptr<CompositorImageDescription> m_preferred;
    std::deque<std::unique_ptr<CompositorImageDescription>> m_pending;
};

class ColorManagementSurface : public QObject, 
                              public QtWayland::wp_color_management_surface_v1
{
    Q_OBJECT

public:
    enum class ColorMode {
        Default = 0,
        sRGB_Gamma22 = 1,
        BT2020_Gamma22 = 2,
        BT2020_PQ = 3,
        PAL_M = 4,
        CIE1931_XYZ = 5
    };

    explicit ColorManagementSurface(ColorManagementGlobal *global,
                                  QQuickWindow *window,
                                  ::wp_color_management_surface_v1 *obj,
                                  std::unique_ptr<ColorManagementFeedback> feedback);
    ~ColorManagementSurface() override;

    void setColorMode(ColorMode mode);
    void setPQMode(int referenceLuminance);

    ColorManagementFeedback* feedback() const { return m_feedback.get(); }

private:
    void createParametricDescription(ColorMode mode);
    void createPQDescription(int referenceLuminance);

    ColorManagementGlobal *m_global;
    QQuickWindow *m_window;
    std::unique_ptr<ColorManagementFeedback> m_feedback;
};

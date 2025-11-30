/**
 * CloudMouse SDK - QR Code Generator Implementation
 *
 * QR code generation and rendering implementation with enhanced features
 * and error handling for reliable display output.
 */

#include "./QRCodeManager.h"

namespace CloudMouse::Utils
{

    // ============================================================================
    // INITIALIZATION
    // ============================================================================

    void QRCodeManager::init(LGFX_Sprite *sprite)
    {
        this->sprite = sprite;
        valid = false;
    }

    // ============================================================================
    // QR CODE GENERATION
    // ============================================================================

    void QRCodeManager::create(const char *content)
    {
        // Use default version 6 with low error correction
        create(content, 6, 0);
    }

    void QRCodeManager::create(const char *content, uint8_t version, uint8_t ecc)
    {
        if (!sprite)
        {
            SDK_LOGGER("❌ QRCodeManager: Sprite not initialized");
            valid = false;
            return;
        }

        if (!content || strlen(content) == 0)
        {
            SDK_LOGGER("❌ QRCodeManager: Empty content provided");
            valid = false;
            return;
        }

        // Allocate buffer for QR code data
        uint8_t qrcodeData[qrcode_getBufferSize(version)];

        // Generate QR code
        int8_t result = qrcode_initText(&qrcode, qrcodeData, version, ecc, content);

        if (result != 0)
        {
            SDK_LOGGER("❌ QRCodeManager: Failed to generate QR code (error: %d)\n", result);
            valid = false;
            return;
        }

        valid = true;

        SDK_LOGGER("✅ QRCodeManager: Generated QR code %dx%d for content length %d\n",
                      qrcode.size, qrcode.size, strlen(content));

        // Render to sprite
        renderToSprite();
    }

    // ============================================================================
    // CONFIGURATION
    // ============================================================================

    void QRCodeManager::setOffset(int x, int y)
    {
        offsetX = x;
        offsetY = y;
    }

    void QRCodeManager::setPixelSize(int pixelSize)
    {
        pixelSide = pixelSize > 0 ? pixelSize : 1; // Minimum 1 pixel
    }

    // ============================================================================
    // STATUS FUNCTIONS
    // ============================================================================

    uint8_t QRCodeManager::getSize() const
    {
        return valid ? qrcode.size : 0;
    }

    int QRCodeManager::getPixelSize() const
    {
        return valid ? (qrcode.size * pixelSide) : 0;
    }

    bool QRCodeManager::isValid() const
    {
        return valid;
    }

    // ============================================================================
    // RENDERING
    // ============================================================================

    void QRCodeManager::renderToSprite()
    {
        if (!valid || !sprite)
        {
            return;
        }

        // Render QR code modules to sprite
        for (uint8_t y = 0; y < qrcode.size; y++)
        {
            for (uint8_t x = 0; x < qrcode.size; x++)
            {
                // Get module state (true = black, false = white)
                bool isBlack = qrcode_getModule(&qrcode, x, y);

                // Calculate pixel position
                int pixelX = offsetX + (x * pixelSide);
                int pixelY = offsetY + (y * pixelSide);

                // Choose color based on module state
                uint16_t color = isBlack ? foregroundColor : backgroundColor;

                // Render module as filled rectangle
                sprite->fillRect(pixelX, pixelY, pixelSide, pixelSide, color);
            }
        }
    }

    // ============================================================================
    // STATIC HELPER FUNCTIONS
    // ============================================================================

    String QRCodeManager::generateWiFiQR(const String &ssid, const String &password, const String &security)
    {
        // WiFi QR code format: WIFI:T:WPA;S:NetworkName;P:Password;H:false;;
        String qrContent = "WIFI:T:" + security + ";S:" + ssid + ";P:" + password + ";H:false;;";
        return qrContent;
    }

    String QRCodeManager::generateURLQR(const String &url)
    {
        // Simple URL QR code - just return the URL
        return url;
    }

    String QRCodeManager::generateTextQR(const String &text)
    {
        // Simple text QR code - just return the text
        return text;
    }

} // namespace CloudMouse::Utils

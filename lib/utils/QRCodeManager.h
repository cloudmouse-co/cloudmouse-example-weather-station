/**
 * CloudMouse SDK - QR Code Generator
 *
 * QR code generation utility for display rendering. Useful for WiFi setup,
 * device identification, configuration sharing, and URL encoding.
 *
 * Features:
 * - QR code generation with configurable error correction
 * - Flexible display interface (works with any graphics library)
 * - Configurable positioning and scaling
 * - Memory efficient rendering
 */

#ifndef QRCODEMANAGER_H
#define QRCODEMANAGER_H

#include <Arduino.h>
#include "../hardware/LGFX_ILI9488.h"

// Platform-specific QR code library includes
#ifdef PLATFORMIO
#include <qrcode.h>
#else
#include "QRCode.h"
#endif

namespace CloudMouse::Utils
{
    /**
     * QR Code Generator and Renderer
     *
     * Generates QR codes and renders them to display sprites or graphics contexts.
     * Supports configurable positioning, scaling, and error correction levels.
     */
    class QRCodeManager
    {
    public:
        /**
         * Initialize QR code manager with display sprite
         *
         * @param sprite Pointer to display sprite for rendering
         */
        void init(LGFX_Sprite *sprite);

        /**
         * Generate and render QR code from text content
         *
         * @param content Text content to encode (URL, WiFi credentials, etc.)
         */
        void create(const char *content);

        /**
         * Generate QR code with custom error correction level
         *
         * @param content Text content to encode
         * @param version QR code version (1-40, higher = more data capacity)
         * @param ecc Error correction level (0=Low, 1=Medium, 2=Quartile, 3=High)
         */
        void create(const char *content, uint8_t version, uint8_t ecc = 0);

        /**
         * Set rendering position offset
         *
         * @param x X coordinate offset from sprite origin
         * @param y Y coordinate offset from sprite origin
         */
        void setOffset(int x, int y);

        /**
         * Set pixel size for QR code modules
         *
         * @param pixelSize Size of each QR code module in pixels
         */
        void setPixelSize(int pixelSize);

        /**
         * Get QR code dimensions
         *
         * @return Size of QR code in modules (width/height are equal)
         */
        uint8_t getSize() const;

        /**
         * Get rendered QR code dimensions in pixels
         *
         * @return Size of rendered QR code in pixels
         */
        int getPixelSize() const;

        /**
         * Check if QR code generation was successful
         *
         * @return true if QR code is valid and ready for rendering
         */
        bool isValid() const;

        // Common QR code content generators
        static String generateWiFiQR(const String &ssid, const String &password, const String &security = "WPA");
        static String generateURLQR(const String &url);
        static String generateTextQR(const String &text);

    private:
        QRCode qrcode;                 // QR code data structure
        LGFX_Sprite *sprite = nullptr; // Display sprite for rendering

        // Rendering configuration
        int offsetX = 0;    // X position offset
        int offsetY = 0;    // Y position offset
        int pixelSide = 3;  // Size of each QR module in pixels
        bool valid = false; // QR code generation status

        // Colors (configurable)
        uint16_t foregroundColor = 0x0000; // Black (TFT_BLACK)
        uint16_t backgroundColor = 0xFFFF; // White (TFT_WHITE)

        // Internal rendering function
        void renderToSprite();
    };
};
#endif
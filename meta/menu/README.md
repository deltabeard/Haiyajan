# menu

Example images of the menu interface, used for testing and demonstrating.

## Testing

The user interface is designed to dynamically scale with DPI, and then limited
by the resolution of the display (render target).

The user interface must be capable of rendering to the following DPI (dot/in)
values:
- 72 (UI elements will be small to compensate small DPI)
- 96
- 150
- 300
- 600 (UI elements will be large to compensate for high DPI)


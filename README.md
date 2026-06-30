# Prevue

An EFL image previewer with a minimap.

![Prevue screenshot](https://github.com/user-attachments/assets/d113154c-8137-4b9c-a953-0369830841c8)

---

Prevue is a minimal, single-instance image previewer built on the Enlightenment Foundation Libraries. It has a minimap overlay when zoomed in, so you always know where you are in the image.

---

## Features

- Bilinear smooth image scaling 
- Opens images at 1:1 for files up to 1280×720, larger images scale down proportionally to fit
- Mouse wheel zoom
- Mouse drag to pan left/top/right/bottom
- Minimap overlay with draggable viewport indicator when in zoom
- Single-instance IPC: opening a second file forwards it to the running instance

## Dependencies

- [EFL](https://www.enlightenment.org) — Elementary, Evas, Edje, Ecore, Ecore-Ipc, Eet
- [Meson](https://mesonbuild.com) build system

## Building

```bash
meson setup build
ninja -C build
sudo ninja -C build install
```

## Usage

```bash
prevue <image>
```

Prevue can also be launched without an argument - it opens at a minimum window size with the IPC server ready.

### From the file manager or desktop

`ninja install` installs the `.desktop` file alongside the binary, so Prevue shows up in Applications -> Graphics menu automatically. If you set it as the default handler for your image formats, opening an image is just double-click (or select + Enter).

### Single-instance behavior

Only one Prevue instance ever runs. Opening a file checks for a running instance via IPC and forwards the path to it rather than spawning a new window. The existing window resizes and re-fits to the new image.

Supported formats depend on the EFL loaders available on your system. Common formats include JPEG, PNG, GIF, WebP, and SVG. Video files with embedded thumbnails may also render depending on your EFL build.

## License

BSD 2-Clause. See [LICENSE](LICENSE).

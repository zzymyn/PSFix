# Photoshop Crash Fix

Stops Photoshop crashing when you have more than one GPU with GPU acceleration.

For example, if you have an AMD card and Intel HD integrated graphics card enabled at the same time, Photoshop will crash if you attempt to turn on GPU acceleration. This plugin/launcher fixes that my hooking Photoshop and hiding the Intel HD card from it.

If you're getting a crash in `atig6txx.dll`, this may fix that problem.

## How to Use

When you download the release, there are two files, `PSFix.8bp` and `PSFixLauncher.exe`. There are two ways to use PSFix, either as a Photoshop plugin or by using the launcher. The plugin has a limitation where OpenCL will be disabled, so if you need/want OpenCL, you have to use the launcher.

### Photoshop Plugin

Simply copy `PSFix.8bp` into your Photoshop plugins folder (`C:\Program Files\Adobe\Adobe Photoshop CC 2018\Plug-ins` for CC 2018).

### Launcher

Run `PSFixLauncher.exe` to launch Photoshop with the fix injected. Note that `PSFixLauncher.exe` still uses the `PSFix.8bp` file so keep that next to `PSFixLauncher.exe`.

## Technical Details

This plugin works by intercepting calls to `DXGIFactory::EnumAdapters()` to hide all but the first adapter from Photoshop. It seems that when Photoshop attempts to initialize more than one graphics card, some graphics card drivers will crash.

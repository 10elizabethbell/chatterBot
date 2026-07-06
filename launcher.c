/* Menu-bar launcher for build/WhisperFlow.app.
 *
 * Runs the whisperflow package in-process via libpython (py2app-style)
 * instead of exec'ing the venv interpreter: on macOS 26 the window server
 * refuses to place a status item for a bundle-launched process that has
 * exec'd a binary other than its declared CFBundleExecutable — the item
 * is silently parked off-screen at x=-1. See CLAUDE.md ("The .app wrapper").
 *
 * Built by build.sh, which links against the uv-managed CPython that
 * backs .venv. The venv is attached via PYTHONPATH since embedded
 * interpreters skip pyvenv.cfg detection.
 */
#include <limits.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern int Py_BytesMain(int argc, char **argv);

static void chop_last_component(char *path) {
    char *slash = strrchr(path, '/');
    if (slash != NULL) {
        *slash = '\0';
    }
}

int main(int argc, char **argv) {
    char exe[PATH_MAX];
    uint32_t size = sizeof(exe);
    if (_NSGetExecutablePath(exe, &size) != 0) {
        return 1;
    }
    char root[PATH_MAX];
    if (realpath(exe, root) == NULL) {
        return 1;
    }
    /* <root>/build/WhisperFlow.app/Contents/MacOS/WhisperFlow */
    for (int i = 0; i < 5; i++) {
        chop_last_component(root);
    }

    char site[PATH_MAX];
    snprintf(site, sizeof(site), "%s/.venv/lib/python3.12/site-packages", root);
    if (access(site, F_OK) != 0) {
        system("osascript -e 'display alert \"WhisperFlow\" message "
               "\"No virtualenv found. Run: uv venv --python 3.12 && "
               "uv pip install -e . in the whisperFlow project.\" as critical'");
        return 1;
    }
    /* The project root supplies the whisperflow package itself: the venv
     * only has an editable-install .pth, which PYTHONPATH doesn't honor. */
    char pythonpath[PATH_MAX * 2];
    snprintf(pythonpath, sizeof(pythonpath), "%s:%s", root, site);
    setenv("PYTHONPATH", pythonpath, 1);

    /* LaunchServices provides a minimal PATH; the cleanup pass locates the
     * `claude` CLI (homebrew) via shutil.which. */
    const char *path = getenv("PATH");
    char newpath[PATH_MAX * 4];
    snprintf(newpath, sizeof(newpath), "/opt/homebrew/bin:/usr/local/bin:%s",
             path != NULL ? path : "/usr/bin:/bin");
    setenv("PATH", newpath, 1);

    /* sys.executable points back at this launcher, so children spawned by
     * multiprocessing etc. arrive here with interpreter args — forward them
     * instead of starting a second app. */
    if (argc > 1) {
        return Py_BytesMain(argc, argv);
    }

    char *py_argv[] = {
        argv[0],
        "-c",
        "from whisperflow.__main__ import main; main()",
        NULL,
    };
    return Py_BytesMain(3, py_argv);
}

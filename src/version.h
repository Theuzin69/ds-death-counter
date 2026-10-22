#pragma once
// Single source of the version number: the exe resources, the About tab and the installer
// (which reads it back from the built exe) all use these values. CI checks that a release
// tag "vX.Y.Z" matches DSDC_VERSION_STRING.
#define DSDC_VERSION_MAJOR 1
#define DSDC_VERSION_MINOR 2
#define DSDC_VERSION_PATCH 0
#define DSDC_VERSION_STRING "1.2.0"

#define DSDC_REPO_URL "https://github.com/Theuzin69/ds-death-counter"

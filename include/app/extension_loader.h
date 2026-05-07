#ifndef GLDRAW_APP_EXTENSION_LOADER_H
#define GLDRAW_APP_EXTENSION_LOADER_H

#include <app/object_manifest.h>

/* Compatibility shim for older object-extension registration naming.
   New code should prefer object_manifest_*(). */
int extension_loader_register_builtins(void);
int extension_loader_register_fake_star(void);
int extension_loader_register_all(void);

#endif /* GLDRAW_APP_EXTENSION_LOADER_H */

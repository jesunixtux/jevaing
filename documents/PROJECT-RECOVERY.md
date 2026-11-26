# Project recovery / Recuperación de proyectos

Source-code recovery from a compiled executable is not implemented and cannot recreate the original source reliably. Project and scene saves retain `.bak` files. Loading falls back to a valid backup when the main file is missing or fails validation; it does not overwrite the main file. The backup may be older than the main file. Inspect it before saving again. This is not a source archive.

The planned optional recovery package would contain the original project encrypted with authenticated encryption and a digital signature. Private decryption/signing keys must stay outside the executable. Key formats, storage, rotation and a recovery test need design and review before implementation.

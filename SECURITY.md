# Security / Seguridad

Report security issues privately to the repository maintainer through GitHub's private vulnerability reporting, when available. Avoid publishing exploit details before a fix.

Jevaing projects can contain CMake and C/C++ code. Building an untrusted project executes that code with the developer's privileges. The editor therefore requires an explicit trust checkbox for each opened project; this is consent, not sandboxing. The optional native module loader executes DLLs with the host process's privileges; manifests and ABI checks do not isolate them.

Project metadata and scene asset paths are confined to their respective roots during loading and export. Project and scene saves use sibling temporary files and a `.bak` copy. This is recovery from interrupted replacement, not a substitute for version control or durable filesystem synchronization. A corrupt existing file is not automatically repaired from backup.

Do not commit private SDKs, credentials or signing keys. Encrypted source recovery is pending; no private key should be embedded in a game executable.

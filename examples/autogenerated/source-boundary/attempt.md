# Keep Java out of the application source

Contract: the exact IB tree/index contains no first-party Java/Kotlin sources,
Gradle source/configuration, or active Java compiler recipes. This is a source
absence gate, independent of application behavior, build or device acceptance.
Inputs are an explicit repository and current paths; output is PASS or refusal
with a nonzero status. Grease owns the OS-visible inspection.

The control uses a temporary Git fixture with the actual required native/core
files. A Java path and a javac recipe must each fail the real guard with the
intended diagnostic. Each clean recovery must pass. No Java source is compiled
or selected as a fallback. The actual IB tree is checked separately.

.class public LReaderTextObserver;
.super Ljava/lang/Object;

# Independent observer only. It creates a real TextView and measures Android's
# actual Layout; the checked candidate owns all text and wrapping configuration.
.method public static main([Ljava/lang/String;)V
    .registers 12
    sget-object v0, Ljava/lang/System;->out:Ljava/io/PrintStream;
    const-string v1, "STAGE native reader observer entered"
    invoke-virtual {v0, v1}, Ljava/io/PrintStream;->println(Ljava/lang/String;)V
    invoke-static {}, Landroid/os/Looper;->prepareMainLooper()V
    # app_process is outside the normal application bind that initializes the
    # platform's lazy font map. Load Android's real fonts, never a test substitute.
    invoke-static {}, Landroid/graphics/Typeface;->loadPreinstalledSystemFontMap()V
    invoke-static {}, Landroid/app/ActivityThread;->systemMain()Landroid/app/ActivityThread;
    move-result-object v6
    const-string v3, "com.android.shell"
    sget-object v4, Landroid/content/res/CompatibilityInfo;->DEFAULT_COMPATIBILITY_INFO:Landroid/content/res/CompatibilityInfo;
    const/4 v5, 0
    invoke-virtual {v6, v3, v4, v5}, Landroid/app/ActivityThread;->getPackageInfo(Ljava/lang/String;Landroid/content/res/CompatibilityInfo;I)Landroid/app/LoadedApk;
    move-result-object v7
    # Bind a real Application for the actual shell package. A bare app Context
    # has no Application until LoadedApk performs its normal initialization.
    const-string v3, "android.app.LoadedApk"
    invoke-static {v3}, Ljava/lang/Class;->forName(Ljava/lang/String;)Ljava/lang/Class;
    move-result-object v3
    const-string v4, "makeApplicationInner"
    const/4 v5, 2
    new-array v5, v5, [Ljava/lang/Class;
    const/4 v8, 0
    sget-object v9, Ljava/lang/Boolean;->TYPE:Ljava/lang/Class;
    aput-object v9, v5, v8
    const/4 v8, 1
    const-class v9, Landroid/app/Instrumentation;
    aput-object v9, v5, v8
    invoke-virtual {v3, v4, v5}, Ljava/lang/Class;->getDeclaredMethod(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;
    move-result-object v3
    const/4 v4, 1
    invoke-virtual {v3, v4}, Ljava/lang/reflect/AccessibleObject;->setAccessible(Z)V
    const/4 v4, 2
    new-array v4, v4, [Ljava/lang/Object;
    const/4 v5, 0
    sget-object v9, Ljava/lang/Boolean;->TRUE:Ljava/lang/Boolean;
    aput-object v9, v4, v5
    invoke-virtual {v3, v7, v4}, Ljava/lang/reflect/Method;->invoke(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;
    move-result-object v7
    check-cast v7, Landroid/app/Application;
    invoke-virtual {v7}, Landroid/content/ContextWrapper;->getBaseContext()Landroid/content/Context;
    move-result-object v0
    invoke-virtual {v0}, Landroid/content/Context;->getOpPackageName()Ljava/lang/String;
    move-result-object v3
    const-string v4, "com.android.shell"
    invoke-virtual {v4, v3}, Ljava/lang/String;->equals(Ljava/lang/Object;)Z
    move-result v3
    if-eqz v3, :observer_identity_failure
    invoke-virtual {v0}, Landroid/content/Context;->getApplicationContext()Landroid/content/Context;
    move-result-object v3
    if-nez v3, :observer_context_ready
    :observer_identity_failure
    const-string v1, "FAIL observer application attribution"
    goto :failure
    :observer_context_ready
    const-class v3, Landroid/app/ActivityThread;
    const-string v4, "mInitialApplication"
    invoke-virtual {v3, v4}, Ljava/lang/Class;->getDeclaredField(Ljava/lang/String;)Ljava/lang/reflect/Field;
    move-result-object v3
    const/4 v4, 1
    invoke-virtual {v3, v4}, Ljava/lang/reflect/AccessibleObject;->setAccessible(Z)V
    invoke-virtual {v3, v6, v7}, Ljava/lang/reflect/Field;->set(Ljava/lang/Object;Ljava/lang/Object;)V
    new-instance v1, Landroid/widget/TextView;
    invoke-direct {v1, v0}, Landroid/widget/TextView;-><init>(Landroid/content/Context;)V
    sget-object v3, Ljava/lang/System;->out:Ljava/io/PrintStream;
    const-string v4, "STAGE framework TextView constructed"
    invoke-virtual {v3, v4}, Ljava/io/PrintStream;->println(Ljava/lang/String;)V

    # Begin with unsuitable platform state so omissions cannot pass by default.
    const/4 v0, 1
    invoke-virtual {v1, v0}, Landroid/widget/TextView;->setSingleLine(Z)V
    invoke-virtual {v1, v0}, Landroid/widget/TextView;->setHorizontallyScrolling(Z)V
    const/4 v0, 0
    invoke-virtual {v1, v0}, Landroid/widget/TextView;->setTextIsSelectable(Z)V

    const-string v2, "\tExact retained text λ😀\n\nReading preserves the source while Android wraps this deliberately long paragraph against the parent width. Reading preserves the source while Android wraps this deliberately long paragraph against the parent width. Reading preserves the source while Android wraps this deliberately long paragraph against the parent width.\n  trailing spaces  \n"
    invoke-static {v1, v2}, LIdric/Generated;->reader_text(Landroid/widget/TextView;Ljava/lang/String;)V
    invoke-static {v1}, LIdric/Generated;->read_reader_text(Landroid/widget/TextView;)Ljava/lang/String;
    move-result-object v3
    invoke-virtual {v2, v3}, Ljava/lang/String;->equals(Ljava/lang/Object;)Z
    move-result v0
    if-eqz v0, :exact_failure
    invoke-virtual {v1}, Landroid/widget/TextView;->isTextSelectable()Z
    move-result v0
    if-eqz v0, :selection_failure

    # Replace the same view with empty text; stale display must not survive.
    const-string v3, ""
    invoke-static {v1, v3}, LIdric/Generated;->reader_text(Landroid/widget/TextView;Ljava/lang/String;)V
    invoke-static {v1}, LIdric/Generated;->read_reader_text(Landroid/widget/TextView;)Ljava/lang/String;
    move-result-object v4
    invoke-virtual {v3, v4}, Ljava/lang/String;->equals(Ljava/lang/Object;)Z
    move-result v0
    if-eqz v0, :exact_failure
    invoke-static {v1, v2}, LIdric/Generated;->reader_text(Landroid/widget/TextView;Ljava/lang/String;)V

    # EXACTLY 240 physical pixels versus EXACTLY 720. These are observer widths,
    # not product dimensions, font assumptions, or IB code-point coordinates.
    const v3, 0x400000f0
    const/4 v4, 0
    invoke-virtual {v1, v3, v4}, Landroid/view/View;->measure(II)V
    invoke-virtual {v1}, Landroid/widget/TextView;->getLineCount()I
    move-result v5
    const v3, 0x400002d0
    invoke-virtual {v1, v3, v4}, Landroid/view/View;->measure(II)V
    invoke-virtual {v1}, Landroid/widget/TextView;->getLineCount()I
    move-result v6
    if-le v5, v6, :reflow_failure
    invoke-static {v1}, LIdric/Generated;->read_reader_text(Landroid/widget/TextView;)Ljava/lang/String;
    move-result-object v3
    invoke-virtual {v2, v3}, Ljava/lang/String;->equals(Ljava/lang/Object;)Z
    move-result v0
    if-eqz v0, :exact_failure

    sget-object v0, Ljava/lang/System;->out:Ljava/io/PrintStream;
    const-string v1, "PASS exact native reader text, selection, empty replacement and width reflow"
    invoke-virtual {v0, v1}, Ljava/io/PrintStream;->println(Ljava/lang/String;)V
    const-string v1, "narrow line count:"
    invoke-virtual {v0, v1}, Ljava/io/PrintStream;->println(Ljava/lang/String;)V
    invoke-virtual {v0, v5}, Ljava/io/PrintStream;->println(I)V
    const-string v1, "wide line count:"
    invoke-virtual {v0, v1}, Ljava/io/PrintStream;->println(Ljava/lang/String;)V
    invoke-virtual {v0, v6}, Ljava/io/PrintStream;->println(I)V
    return-void

    :exact_failure
    const-string v1, "FAIL exact native reader text"
    goto :failure
    :selection_failure
    const-string v1, "FAIL native reader selection"
    goto :failure
    :reflow_failure
    const-string v1, "FAIL native reader width reflow"
    :failure
    sget-object v0, Ljava/lang/System;->err:Ljava/io/PrintStream;
    invoke-virtual {v0, v1}, Ljava/io/PrintStream;->println(Ljava/lang/String;)V
    const/16 v0, 73
    invoke-static {v0}, Ljava/lang/System;->exit(I)V
    return-void
.end method

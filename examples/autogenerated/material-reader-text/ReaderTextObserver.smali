.class public LReaderTextObserver;
.super Ljava/lang/Object;

# Independent observer only. It creates a real TextView and measures Android's
# actual Layout; the checked candidate owns all text and wrapping configuration.
.method public static main([Ljava/lang/String;)V
    .registers 9
    invoke-static {}, Landroid/os/Looper;->prepareMainLooper()V
    invoke-static {}, Landroid/app/ActivityThread;->systemMain()Landroid/app/ActivityThread;
    move-result-object v0
    invoke-virtual {v0}, Landroid/app/ActivityThread;->getSystemContext()Landroid/app/ContextImpl;
    move-result-object v0
    new-instance v1, Landroid/widget/TextView;
    invoke-direct {v1, v0}, Landroid/widget/TextView;-><init>(Landroid/content/Context;)V

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

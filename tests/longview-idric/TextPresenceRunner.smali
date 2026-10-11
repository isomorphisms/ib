.class public final LIdric/TextPresenceRunner;
.super Ljava/lang/Object;

# External ART oracle for the checked Idriç LongViewTextProbe candidate.
# Only the actual source-compiled LIdric/Generated; provides the operations.
# The runner itself never supplies browser/validator implementation.

.method private static assert_expected(III)V
    .registers 3
    if-eq p0, p1, :pass
    invoke-static {p2}, Ljava/lang/System;->exit(I)V
  :pass
    return-void
.end method

.method public static main([Ljava/lang/String;)V
    .registers 6

    # safe ASCII
    const-string v0, "abc"
    invoke-static {v0}, LIdric/Generated;->record_identity_allowed(Ljava/lang/String;)I
    move-result v1
    const/4 v2, 0x1
    const/16 v3, 0x1f
    invoke-static {v1, v2, v3}, LIdric/TextPresenceRunner;->assert_expected(III)V

    # empty
    const-string v0, ""
    invoke-static {v0}, LIdric/Generated;->record_identity_allowed(Ljava/lang/String;)I
    move-result v1
    const/4 v2, 0x0
    const/16 v3, 0x20
    invoke-static {v1, v2, v3}, LIdric/TextPresenceRunner;->assert_expected(III)V

    # tab
    const-string v0, "alpha\\tbeta"
    invoke-static {v0}, LIdric/Generated;->record_identity_allowed(Ljava/lang/String;)I
    move-result v1
    const/4 v2, 0x0
    const/16 v3, 0x21
    invoke-static {v1, v2, v3}, LIdric/TextPresenceRunner;->assert_expected(III)V

    # line feed
    const-string v0, "alpha\\nbeta"
    invoke-static {v0}, LIdric/Generated;->record_identity_allowed(Ljava/lang/String;)I
    move-result v1
    const/4 v2, 0x0
    const/16 v3, 0x22
    invoke-static {v1, v2, v3}, LIdric/TextPresenceRunner;->assert_expected(III)V

    # carriage return
    const-string v0, "alpha\\rbeta"
    invoke-static {v0}, LIdric/Generated;->record_identity_allowed(Ljava/lang/String;)I
    move-result v1
    const/4 v2, 0x0
    const/16 v3, 0x23
    invoke-static {v1, v2, v3}, LIdric/TextPresenceRunner;->assert_expected(III)V

    # non-ASCII
    const-string v0, "pi=π,漢字"
    invoke-static {v0}, LIdric/Generated;->record_identity_allowed(Ljava/lang/String;)I
    move-result v1
    const/4 v2, 0x1
    const/16 v3, 0x24
    invoke-static {v1, v2, v3}, LIdric/TextPresenceRunner;->assert_expected(III)V

    # identity question mark permitted
    const-string v0, "a?b"
    invoke-static {v0}, LIdric/Generated;->record_identity_allowed(Ljava/lang/String;)I
    move-result v1
    const/4 v2, 0x1
    const/16 v3, 0x25
    invoke-static {v1, v2, v3}, LIdric/TextPresenceRunner;->assert_expected(III)V

    # identity fragment marker permitted
    const-string v0, "a#b"
    invoke-static {v0}, LIdric/Generated;->record_identity_allowed(Ljava/lang/String;)I
    move-result v1
    const/4 v2, 0x1
    const/16 v3, 0x26
    invoke-static {v1, v2, v3}, LIdric/TextPresenceRunner;->assert_expected(III)V

    # neutral origin
    const-string v0, "https://example.test"
    invoke-static {v0}, LIdric/Generated;->neutral_address_allowed(Ljava/lang/String;)I
    move-result v1
    const/4 v2, 0x1
    const/16 v3, 0x27
    invoke-static {v1, v2, v3}, LIdric/TextPresenceRunner;->assert_expected(III)V

    # neutral empty
    const-string v0, ""
    invoke-static {v0}, LIdric/Generated;->neutral_address_allowed(Ljava/lang/String;)I
    move-result v1
    const/4 v2, 0x0
    const/16 v3, 0x28
    invoke-static {v1, v2, v3}, LIdric/TextPresenceRunner;->assert_expected(III)V

    # neutral query
    const-string v0, "https://example.test/?code=secret"
    invoke-static {v0}, LIdric/Generated;->neutral_address_allowed(Ljava/lang/String;)I
    move-result v1
    const/4 v2, 0x0
    const/16 v3, 0x29
    invoke-static {v1, v2, v3}, LIdric/TextPresenceRunner;->assert_expected(III)V

    # neutral fragment
    const-string v0, "https://example.test/#section"
    invoke-static {v0}, LIdric/Generated;->neutral_address_allowed(Ljava/lang/String;)I
    move-result v1
    const/4 v2, 0x0
    const/16 v3, 0x2a
    invoke-static {v1, v2, v3}, LIdric/TextPresenceRunner;->assert_expected(III)V

    # neutral tab
    const-string v0, "https://example.test\\tbad"
    invoke-static {v0}, LIdric/Generated;->neutral_address_allowed(Ljava/lang/String;)I
    move-result v1
    const/4 v2, 0x0
    const/16 v3, 0x2b
    invoke-static {v1, v2, v3}, LIdric/TextPresenceRunner;->assert_expected(III)V

    # neutral unicode
    const-string v0, "https://example.test/é"
    invoke-static {v0}, LIdric/Generated;->neutral_address_allowed(Ljava/lang/String;)I
    move-result v1
    const/4 v2, 0x1
    const/16 v3, 0x2c
    invoke-static {v1, v2, v3}, LIdric/TextPresenceRunner;->assert_expected(III)V

    sget-object v0, Ljava/lang/System;->out:Ljava/io/PrintStream;
    const-string v1, "IB checked DEX text presence ART PASS"
    invoke-virtual {v0, v1}, Ljava/io/PrintStream;->println(Ljava/lang/String;)V
    return-void
.end method

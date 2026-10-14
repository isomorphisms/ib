#import <AppKit/AppKit.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

/*
 * Disposable reader of IB's existing ib-prepaint v1 interchange.
 * This file does not fetch, choose a URL, or own browser identity/history.
 * Unsupported interactive/image records fail closed instead of vanishing.
 */
enum { IBMaxBytes = 4 * 1024 * 1024, IBMaxRevisions = 32, IBMaxBlocks = 4096 };

static NSString *IBUnescape(NSString *encoded)
{
    NSMutableString *decoded = [NSMutableString new];
    for (NSUInteger index = 0; index < encoded.length; ++index) {
        unichar character = [encoded characterAtIndex:index];
        if (character == 0 || character == '\r') {
            return nil;
        }
        if (character != '\\') {
            [decoded appendFormat:@"%C", character];
            continue;
        }
        if (++index == encoded.length) {
            return nil;
        }
        switch ([encoded characterAtIndex:index]) {
        case '\\': [decoded appendString:@"\\"]; break;
        case 't': [decoded appendString:@"\t"]; break;
        case 'n': [decoded appendString:@"\n"]; break;
        case 'r': [decoded appendString:@"\r"]; break;
        default: return nil;
        }
    }
    return decoded;
}

static BOOL IBRevisionNumber(NSString *text, unsigned long long *value)
{
    if (text.length == 0) {
        return NO;
    }
    unsigned long long result = 0;
    for (NSUInteger index = 0; index < text.length; ++index) {
        unichar digit = [text characterAtIndex:index];
        if (digit < '0' || digit > '9') {
            return NO;
        }
        unsigned int number = (unsigned int)(digit - '0');
        if (result > (ULLONG_MAX - number) / 10) {
            return NO;
        }
        result = result * 10 + number;
    }
    *value = result;
    return YES;
}

static BOOL IBParsePrepaint(NSData *bytes, NSString **text,
                            unsigned long long *lastRevision,
                            NSUInteger *lastBlockCount, NSString **lastState)
{
    if (bytes.length == 0 || bytes.length > IBMaxBytes ||
        memchr(bytes.bytes, '\0', bytes.length) != NULL) {
        return NO;
    }
    NSString *source = [[NSString alloc] initWithData:bytes encoding:NSUTF8StringEncoding];
    if (source == nil) {
        return NO;
    }
    NSArray<NSString *> *lines = [source componentsSeparatedByString:@"\n"];
    if (lines.count == 0 || ![lines[0] isEqualToString:@"ib-prepaint\t1"]) {
        return NO;
    }

    BOOL inside = NO;
    NSUInteger revisionCount = 0;
    NSUInteger blockCount = 0;
    unsigned long long revision = 0;
    NSMutableString *current = nil;
    NSString *state = nil;
    NSString *committed = nil;
    NSUInteger committedBlocks = 0;

    for (NSUInteger lineIndex = 1; lineIndex < lines.count; ++lineIndex) {
        NSString *line = lines[lineIndex];
        if (lineIndex + 1 == lines.count && line.length == 0) {
            break; /* one final LF */
        }
        if (line.length == 0 || [line rangeOfString:@"\r"].location != NSNotFound) {
            return NO;
        }
        NSArray<NSString *> *fields = [line componentsSeparatedByString:@"\t"];
        NSString *tag = fields[0];
        if (!inside) {
            unsigned long long nextRevision = 0;
            if (![tag isEqualToString:@"revision"] || fields.count != 3 ||
                !IBRevisionNumber(fields[1], &nextRevision) ||
                nextRevision <= revision || revisionCount >= IBMaxRevisions ||
                !([fields[2] isEqualToString:@"partial"] ||
                  [fields[2] isEqualToString:@"complete"])) {
                return NO;
            }
            revision = nextRevision;
            ++revisionCount;
            blockCount = 0;
            current = [NSMutableString new];
            state = fields[2];
            inside = YES;
            continue;
        }
        if ([tag isEqualToString:@"end"]) {
            if (fields.count != 1) {
                return NO;
            }
            committed = [current copy];
            committedBlocks = blockCount;
            inside = NO;
            continue;
        }
        if ([tag isEqualToString:@"requested-url"] ||
            [tag isEqualToString:@"resolved-url"]) {
            if (fields.count != 2 || IBUnescape(fields[1]) == nil) {
                return NO;
            }
            continue; /* addresses are core metadata, not invented page content */
        }
        if (++blockCount > IBMaxBlocks) {
            return NO;
        }
        if ([tag isEqualToString:@"title"] || [tag isEqualToString:@"text"]) {
            if (fields.count != 2) {
                return NO;
            }
            NSString *value = IBUnescape(fields[1]);
            if (value == nil) {
                return NO;
            }
            [current appendFormat:@"%@\n", value];
        } else if ([tag isEqualToString:@"heading"]) {
            if (fields.count != 3 || fields[1].length != 1) {
                return NO;
            }
            unichar level = [fields[1] characterAtIndex:0];
            NSString *value = IBUnescape(fields[2]);
            if (level < '1' || level > '6' || value == nil) {
                return NO;
            }
            NSString *marks = [@"" stringByPaddingToLength:(NSUInteger)(level - '0')
                                                withString:@"#" startingAtIndex:0];
            [current appendFormat:@"%@ %@\n", marks, value];
        } else if ([tag isEqualToString:@"link"]) {
            if (fields.count != 3) {
                return NO;
            }
            NSString *label = IBUnescape(fields[1]);
            NSString *target = IBUnescape(fields[2]);
            if (label == nil || target == nil) {
                return NO;
            }
            /* Show/copy the target; never navigate outside IB's core. */
            [current appendFormat:@"%@  <%@>\n", label, target];
        } else if ([tag isEqualToString:@"row"]) {
            if (fields.count < 2) {
                return NO;
            }
            NSMutableArray<NSString *> *cells = [NSMutableArray new];
            for (NSUInteger index = 1; index < fields.count; ++index) {
                NSString *cell = IBUnescape(fields[index]);
                if (cell == nil) {
                    return NO;
                }
                [cells addObject:cell];
            }
            [current appendFormat:@"%@\n", [cells componentsJoinedByString:@"  |  "]];
        } else {
            /* form, image and unknown records need a real adapter, not silent loss */
            return NO;
        }
    }
    if (inside || revisionCount == 0 || committed == nil) {
        return NO;
    }
    *text = committed;
    *lastRevision = revision;
    *lastBlockCount = committedBlocks;
    *lastState = state;
    return YES;
}

@interface IBAppDelegate : NSObject <NSApplicationDelegate>
@property(nonatomic, strong) NSWindow *window;
@property(nonatomic, copy) NSString *projection;
@end

@implementation IBAppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    (void)notification;
    NSRect frame = NSMakeRect(0, 0, 880, 640);
    self.window = [[NSWindow alloc] initWithContentRect:frame
        styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                   NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable)
          backing:NSBackingStoreBuffered defer:NO];
    self.window.title = @"IB · macOS prepaint (prototype)";
    NSScrollView *scroll = [[NSScrollView alloc] initWithFrame:frame];
    scroll.hasVerticalScroller = YES;
    scroll.autohidesScrollers = YES;
    NSTextView *viewer = [[NSTextView alloc] initWithFrame:frame];
    viewer.editable = NO;
    viewer.selectable = YES;
    viewer.verticallyResizable = YES;
    viewer.horizontallyResizable = NO;
    viewer.autoresizingMask = NSViewWidthSizable;
    viewer.textContainer.widthTracksTextView = YES;
    viewer.font = [NSFont systemFontOfSize:15];
    viewer.string = self.projection;
    scroll.documentView = viewer;
    self.window.contentView = scroll;
    [self.window center];
    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    (void)sender;
    return YES;
}
@end

int main(int argc, const char *argv[])
{
    @autoreleasepool {
        BOOL check = argc == 3 && strcmp(argv[1], "--check") == 0;
        BOOL dump = argc == 3 && strcmp(argv[1], "--dump") == 0;
        if (!((argc == 2) || check || dump)) {
            fprintf(stderr, "usage: IBPrepaint [--check|--dump] file.prepaint\n");
            return 2;
        }
        const char *path = argv[argc - 1];
        NSString *name = [NSString stringWithUTF8String:path];
        NSData *bytes = name ? [NSData dataWithContentsOfFile:name] : nil;
        NSString *projection = nil;
        NSString *state = nil;
        unsigned long long revision = 0;
        NSUInteger blocks = 0;
        if (bytes == nil ||
            !IBParsePrepaint(bytes, &projection, &revision, &blocks, &state)) {
            fprintf(stderr, "invalid or unsupported ib-prepaint v1 input\n");
            return 3;
        }
        if (check) {
            printf("revision=%llu\nblocks=%lu\nstate=%s\n", revision,
                   (unsigned long)blocks, state.UTF8String);
            return 0;
        }
        if (dump) {
            NSData *utf8 = [projection dataUsingEncoding:NSUTF8StringEncoding];
            if (fwrite(utf8.bytes, 1, utf8.length, stdout) != utf8.length) {
                return 4;
            }
            return 0;
        }
        IBAppDelegate *delegate = [IBAppDelegate new];
        delegate.projection = projection;
        NSApplication *app = [NSApplication sharedApplication];
        app.delegate = delegate;
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        [app run];
    }
    return 0;
}

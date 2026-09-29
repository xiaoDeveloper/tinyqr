$ErrorActionPreference = 'Stop'
$render = Get-Content -Raw "$PSScriptRoot/../app/src/main/c/render/render.c"

if ($render -match '\(width - x \* 2\)') {
    throw 'Bounded payload text must derive columns from explicit card content bounds.'
}
if ($render -notmatch 'static void text\([\s\S]*int max_rows, int max_width, uint16_t color\)') {
    throw 'Expected bounded multiline text() to accept an explicit max_width.'
}
if ($render -notmatch 'static void text_single_line\(') {
    throw 'Expected a dedicated single-line label renderer.'
}
foreach ($label in @('title', 'binary', 'label', 'rescan')) {
    if ($render -notmatch "text_single_line\([^;]*\b$label\b") {
        throw "Expected $label to use the single-line label renderer."
    }
}

Write-Output 'render text layout tests passed'

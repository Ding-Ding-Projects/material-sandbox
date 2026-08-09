import fs from 'node:fs';
import path from 'node:path';

const root = process.cwd();
const read = (file) => fs.readFileSync(path.join(root, file), 'utf8');
const cpp = read('SandboxiePlus/SandMan/Windows/AppearanceEditorDialog.cpp');
const h = read('SandboxiePlus/SandMan/Windows/AppearanceEditorDialog.h');
const settings = read('SandboxiePlus/SandMan/Windows/SettingsWindow.cpp');
const startup = read('SandboxiePlus/SandMan/SandMan.cpp');
const pri = read('SandboxiePlus/SandMan/SandMan.pri');
const vcxproj = read('SandboxiePlus/SandMan/SandMan.vcxproj');
const qrc = read('SandboxiePlus/SandMan/Resources/SandMan.qrc');
const docs = read('SandboxiePlus/SandMan/Resources/Docs/appearance-editor.md');
const tabs = read('SandboxiePlus/MiscHelpers/Common/TabStateManager.cpp');

const checks = [
  ['dialog class', h.includes('class CAppearanceEditorDialog') && cpp.includes('CAppearanceEditorDialog::CAppearanceEditorDialog')],
  ['installed font family', cpp.includes('QFontComboBox') && cpp.includes('ScalableFonts')],
  ['font controls', cpp.includes('m_size') && cpp.includes('m_weight') && cpp.includes('m_style')],
  ['Qt typography controls', cpp.includes('m_underline') && cpp.includes('Underline (single line)') && cpp.includes('m_strikeOut') && cpp.includes('m_overline') && cpp.includes('m_capitalization') && cpp.includes('m_letterSpacing') && cpp.includes('m_wordSpacing')],
  ['color controls', cpp.includes('m_textColor') && cpp.includes('m_highlight') && cpp.includes('chooseTextColor') && cpp.includes('chooseHighlightColor')],
  ['live preview', cpp.includes('updatePreview') && cpp.includes('setStyleSheet')],
  ['translator reuse', cpp.includes('CColorTranslatorDialog') && settings.includes('CAppearanceEditorDialog')],
  ['persistent values', settings.includes('UIConfig/UIFontFamily') && settings.includes('UIConfig/UIFontPointSize') && startup.includes('UIFontWeight')],
  ['reset path', cpp.includes('resetToShippedDefaults') && docs.includes('Reset to shipped defaults')],
  ['unsupported disclosure', cpp.includes('Not represented by this global native slice') && docs.includes('does not claim full Word-depth') && docs.includes('line-height') && docs.includes('baseline offset') && docs.includes('underline variants')],
  ['Qt 6.8 variable-axis boundary', cpp.includes('Qt 6.8 can represent font-specific variable axes') && docs.includes('Qt 6.8 can represent font-specific variable axes')],
  ['real per-element target', tabs.includes('QStringLiteral("appearance")') && tabs.includes('tabKey(i)') && docs.includes('stable tab key')],
  ['per-element Qt typography controls', tabs.includes('Tab font weight') && tabs.includes('Tab underline') && tabs.includes('Tab capitalization') && tabs.includes('Tab letter spacing') && tabs.includes('Tab word spacing')],
  ['per-element limitation disclosure', tabs.includes('Line-height, baseline offset, superscript, subscript') && tabs.includes('outline, shadow, and glow') && docs.includes('underline variants')],
  ['per-element target boundary', tabs.includes('Edit tab page typography') && tabs.includes('tab page content, not the tab-bar label') && docs.includes('tab page content rather than its `QTabBar` label')],
  ['qmake registration', pri.includes('./Windows/AppearanceEditorDialog.h') && pri.includes('./Windows/AppearanceEditorDialog.cpp')],
  ['MSVC registration', vcxproj.includes('Windows\\AppearanceEditorDialog.cpp') && vcxproj.includes('Windows\\AppearanceEditorDialog.h')],
  ['offline article bundle', qrc.includes('Docs/appearance-editor.md')],
];
const failed = checks.filter(([, ok]) => !ok).map(([name]) => name);
if (failed.length) {
  console.error(`appearance-editor-contract failed: ${failed.join(', ')}`);
  process.exit(1);
}
console.log(`appearance-editor-contract checks=${checks.length}`);

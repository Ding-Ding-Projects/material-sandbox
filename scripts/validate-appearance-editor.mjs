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

const checks = [
  ['dialog class', h.includes('class CAppearanceEditorDialog') && cpp.includes('CAppearanceEditorDialog::CAppearanceEditorDialog')],
  ['installed font family', cpp.includes('QFontComboBox') && cpp.includes('ScalableFonts')],
  ['font controls', cpp.includes('m_size') && cpp.includes('m_weight') && cpp.includes('m_style')],
  ['live preview', cpp.includes('updatePreview') && cpp.includes('setStyleSheet')],
  ['translator reuse', cpp.includes('CColorTranslatorDialog') && settings.includes('CAppearanceEditorDialog')],
  ['persistent values', settings.includes('UIConfig/UIFontFamily') && settings.includes('UIConfig/UIFontPointSize') && startup.includes('UIFontWeight')],
  ['reset path', cpp.includes('resetToShippedDefaults') && docs.includes('Reset to shipped defaults')],
  ['unsupported disclosure', cpp.includes('Not represented by this native slice') && docs.includes('does not yet claim full Word-depth')],
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

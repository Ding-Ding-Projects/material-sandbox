import fs from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const packagingFiles = [
  'build.bat',
  'build-installer.bat',
  'Installer/Languages.iss',
  'Installer/Sandboxie-Plus.iss',
  'Installer/copy_build.cmd',
  'Installer/merge_builds.cmd',
  'Sandboxie/core/drv/SboxDrv.vcxproj',
  'Sandboxie/install/build.bat',
  'SandboxiePlus/install_jom.cmd',
  'SandboxiePlus/qmake_plus.cmd',
  'scripts/windows-build-bootstrap.ps1',
];

const expectedDriverConfigurations = [
  'SbieDebug|Win32',
  'SbieDebug|x64',
  'SbieDebug|ARM64',
  'SbieRelease|Win32',
  'SbieRelease|x64',
  'SbieRelease|ARM64',
];

const forbiddenRoutes = [
  ['Inno Setup signing directive', /^\s*SignTool\s*=/im],
  ['signing executable invocation', /\bsigntool(?:\.exe)?\b/i],
  ['signing command variable', /\bSIGN(?:ING)?_?(?:CMD|COMMAND|TOOL)\b/i],
  ['WDK DriverSign item-definition route', /<DriverSign(?:\s|>)/i],
  ['Inno Setup command-line signing route', /\s\/S[A-Za-z0-9_-]+\s*=/],
  ['signed-uninstaller output directory', /^\s*SignedUninstallerDir\s*=/im],
  ['active signed-uninstaller directive', /^\s*SignedUninstaller\s*=\s*(?!no(?:\s|;|$))/im],
  ['Inno file-signing flag', /^\s*(?!;).*?\bFlags\s*:\s*[^;\r\n]*\b(?:sign|signonce|signcheck)\b/im],
  ['Inno preprocessor emission route', /^\s*#(?:emit|expr)\b/im],
  ['environment-derived Inno route', /\bGetEnv\s*\(/i],
  ['Authenticode signing invocation', /\bSet-AuthenticodeSignature\b/i],
  ['alternate signing executable', /\b(?:osslsigncode|AzureSignTool|jsign|smctl)(?:\.exe)?\b/i],
  ['runtime Inno setup mutation', /\bSetSetupSetting\s*\(/i],
  ['external Inno include path', /\s\/J\S*/],
  ['certificate-bundle input', /\b(?:PFX|P12|PKCS12|CSC_LINK)\b/i],
  [
    'credential-bearing signing input',
    /\b(?:CODE_?SIGN|AUTHENTICODE|PACKAGE_?SIGNING|SIGNING|CERT(?:IFICATE)?)[A-Z0-9_]*(?:PASSWORD|PASS|PWD|TOKEN|SECRET|KEY)[A-Z0-9_]*\b/i,
  ],
];

function readPackagingFiles(root = process.cwd()) {
  return Object.fromEntries(
    packagingFiles.map((file) => {
      const absolute = path.join(root, file);
      if (!fs.existsSync(absolute)) throw new Error(`unsigned-packaging: missing required file ${file}`);
      return [file, fs.readFileSync(absolute, 'utf8')];
    }),
  );
}

export function unsignedPackagingFailures(sources) {
  const failures = [];

  for (const [file, text] of Object.entries(sources)) {
    for (const [label, pattern] of forbiddenRoutes) {
      if (pattern.test(text)) failures.push(`${file}: contains a forbidden ${label}`);
    }
  }

  const installer = sources['build-installer.bat'];
  const bootstrap = sources['scripts/windows-build-bootstrap.ps1'];
  const installerScript = sources['Installer/Sandboxie-Plus.iss'];
  const driverProject = sources['Sandboxie/core/drv/SboxDrv.vcxproj'];
  const legacyInstaller = sources['Sandboxie/install/build.bat'];

  for (const [file, text] of Object.entries(sources).filter(([name]) => name.endsWith('.iss'))) {
    const includeDirectives = [...text.matchAll(/^\s*#include\b[^\r\n]*/gim)];
    const literalIncludes = [...text.matchAll(/^\s*#include\s+"([^"]+)"/gim)];
    if (includeDirectives.length !== literalIncludes.length) failures.push(`${file}: contains a computed or unresolved include`);
    for (const include of literalIncludes) {
      const includedPath = path.posix.normalize(path.posix.join(path.posix.dirname(file), include[1].replaceAll('\\', '/')));
      if (!(includedPath in sources)) failures.push(`${file}: included script is missing from the audited packaging inventory`);
    }
  }

  for (const marker of [
    'windows-build-bootstrap.ps1',
    '-Mode Installer',
    'permanently unsigned',
  ]) {
    if (!installer.includes(marker)) failures.push(`build-installer.bat: missing unsigned delegation marker ${marker}`);
  }
  for (const marker of [
    "'Sandboxie-Plus.iss'",
    'Get-AuthenticodeSignature',
    "-ne 'NotSigned'",
    'SignerCertificate',
    'TimeStamperCertificate',
    'Select-ExactlyOne',
    "'^[0-9a-f]{64}$'",
    'build-manifest.json',
    'permanently unsigned',
  ]) {
    if (!bootstrap.includes(marker)) failures.push(`scripts/windows-build-bootstrap.ps1: missing unsigned contract marker ${marker}`);
  }
  if (/TEMP_ISS|findstr\s+\/V/i.test(`${installer}\n${bootstrap}`)) {
    failures.push('installer entry points must compile the canonical installer script directly');
  }
  if (!/^\s*\[Setup\]\s*$/im.test(installerScript)) {
    failures.push('Installer/Sandboxie-Plus.iss: missing Setup section');
  }
  if (!/permanently unsigned/i.test(installerScript)) {
    failures.push('Installer/Sandboxie-Plus.iss: missing permanent unsigned policy marker');
  }
  const signedUninstaller = [...installerScript.matchAll(/^\s*SignedUninstaller\s*=\s*([^;\s]+)\s*$/gim)];
  if (signedUninstaller.length !== 1 || signedUninstaller[0][1].toLowerCase() !== 'no') {
    failures.push('Installer/Sandboxie-Plus.iss: SignedUninstaller must be explicitly disabled');
  }

  const driverGroups = [...driverProject.matchAll(/<PropertyGroup\s+Condition="([^"]+)"\s+Label="Configuration">([\s\S]*?)<\/PropertyGroup>/gi)];
  const globalGroup = driverProject.match(/<PropertyGroup\s+Label="Globals">([\s\S]*?)<\/PropertyGroup>/i);
  const globalSigningModes = globalGroup ? [...globalGroup[1].matchAll(/<SignMode>\s*([^<]+?)\s*<\/SignMode>/gi)] : [];
  if (globalSigningModes.length !== 1 || globalSigningModes[0][1].trim() !== 'Off') {
    failures.push('Sandboxie/core/drv/SboxDrv.vcxproj: global driver signing mode must default to Off');
  }
  if (!/<Target\s+Name="EnforceUnsignedDriverPolicy"\s+BeforeTargets="PrepareForBuild">[\s\S]*?<Error\s+Condition="'\$\(SignMode\)' != 'Off'"[^>]*>[\s\S]*?<\/Target>/i.test(driverProject)) {
    failures.push('Sandboxie/core/drv/SboxDrv.vcxproj: missing fail-closed evaluated driver-signing policy target');
  }
  for (const configuration of expectedDriverConfigurations) {
    const group = driverGroups.find((match) => match[1].includes(`'${configuration}'`));
    if (!group) {
      failures.push(`Sandboxie/core/drv/SboxDrv.vcxproj: missing ${configuration} configuration property group`);
      continue;
    }
    const signingModes = [...group[2].matchAll(/<SignMode>\s*([^<]+?)\s*<\/SignMode>/gi)];
    if (signingModes.length !== 1 || signingModes[0][1].trim() !== 'Off') {
      failures.push(`Sandboxie/core/drv/SboxDrv.vcxproj: ${configuration} must set scalar SignMode to Off`);
    }
  }
  if (!/permanently unsigned/i.test(legacyInstaller) || !/exit\s+\/b\s+64/i.test(legacyInstaller)) {
    failures.push('Sandboxie/install/build.bat: obsolete route must fail closed and direct callers to unsigned packaging');
  }

  return failures;
}

export function validateUnsignedPackaging(root = process.cwd()) {
  const failures = unsignedPackagingFailures(readPackagingFiles(root));
  if (failures.length) throw new Error(failures.join('\n'));
  return packagingFiles.length;
}

function selfTest(root) {
  const baseline = readPackagingFiles(root);
  const fixtures = [
    {
      name: 'directive route',
      file: 'Installer/Sandboxie-Plus.iss',
      append: '\nSignTool=fixture-sign-command\n',
    },
    {
      name: 'executable route',
      file: 'build-installer.bat',
      append: '\nsigntool.exe sign fixture.exe\n',
    },
    {
      name: 'signing input',
      file: 'build-installer.bat',
      append: '\nset "PACKAGE_SIGNING_KEY_FILE=fixture"\n',
    },
    {
      name: 'Inno command-line signing route',
      file: 'build-installer.bat',
      append: '\nISCC.exe /Sfixture=fixture-command Installer.iss\n',
    },
    {
      name: 'signed uninstaller route',
      file: 'Installer/Languages.iss',
      append: '\nSignedUninstaller=yes\n',
    },
    {
      name: 'Inno file-signing flag',
      file: 'Installer/Languages.iss',
      append: '\nSource: "fixture.exe"; DestDir: "{app}"; Flags: signonce\n',
    },
    {
      name: 'Inno preprocessor emission route',
      file: 'Installer/Languages.iss',
      append: '\n#emit "fixture"\n',
    },
    {
      name: 'environment-derived Inno route',
      file: 'Installer/Languages.iss',
      append: '\n#define Fixture GetEnv("FIXTURE")\n',
    },
    {
      name: 'Authenticode signing invocation',
      file: 'build-installer.bat',
      append: '\nSet-AuthenticodeSignature -FilePath fixture.exe\n',
    },
    {
      name: 'untracked Inno include',
      file: 'Installer/Sandboxie-Plus.iss',
      append: '\n#include "Fixture.iss"\n',
    },
    {
      name: 'computed Inno include',
      file: 'Installer/Languages.iss',
      append: '\n#include FixtureInclude\n',
    },
    {
      name: 'runtime Inno setup mutation',
      file: 'Installer/Languages.iss',
      append: '\nSetSetupSetting("Fixture", "value")\n',
    },
    {
      name: 'external Inno include path',
      file: 'build-installer.bat',
      append: '\nISCC.exe /Jfixture Installer.iss\n',
    },
    {
      name: 'alternate signing executable',
      file: 'build-installer.bat',
      append: '\nosslsigncode sign fixture.exe\n',
    },
    {
      name: 'missing unsigned proof',
      file: 'scripts/windows-build-bootstrap.ps1',
      replace: ["-ne 'NotSigned'", "-ne 'FixtureState'"],
    },
    ...expectedDriverConfigurations.map((configuration) => ({
      name: `driver signing enabled for ${configuration}`,
      file: 'Sandboxie/core/drv/SboxDrv.vcxproj',
      mutate(text) {
        const conditionIndex = text.indexOf(`'${configuration}'`);
        const groupEnd = text.indexOf('</PropertyGroup>', conditionIndex);
        const mode = '<SignMode>Off</SignMode>';
        const modeIndex = text.indexOf(mode, conditionIndex);
        if (conditionIndex < 0 || groupEnd < 0 || modeIndex < 0 || modeIndex > groupEnd) return text;
        return `${text.slice(0, modeIndex)}<SignMode>TestSign</SignMode>${text.slice(modeIndex + mode.length)}`;
      },
    })),
    {
      name: 'global driver signing default enabled',
      file: 'Sandboxie/core/drv/SboxDrv.vcxproj',
      replace: ['<SignMode>Off</SignMode>', '<SignMode>TestSign</SignMode>'],
    },
    {
      name: 'global driver signing mode omitted',
      file: 'Sandboxie/core/drv/SboxDrv.vcxproj',
      replace: ['<SignMode>Off</SignMode>', ''],
    },
    {
      name: 'WDK DriverSign route restored',
      file: 'Sandboxie/core/drv/SboxDrv.vcxproj',
      append: '\n<DriverSign><FileDigestAlgorithm>SHA256</FileDigestAlgorithm></DriverSign>\n',
    },
    {
      name: 'driver policy gate removed',
      file: 'Sandboxie/core/drv/SboxDrv.vcxproj',
      replace: ['<Target Name="EnforceUnsignedDriverPolicy"', '<Target Name="FixturePolicy"'],
    },
    {
      name: 'driver policy condition inverted',
      file: 'Sandboxie/core/drv/SboxDrv.vcxproj',
      replace: ["Condition=\"'$(SignMode)' != 'Off'\"", "Condition=\"'$(SignMode)' == 'Off'\""],
    },
  ];

  for (const fixture of fixtures) {
    const mutated = { ...baseline };
    if (fixture.append) mutated[fixture.file] += fixture.append;
    if (fixture.replace) mutated[fixture.file] = mutated[fixture.file].replace(...fixture.replace);
    if (fixture.mutate) mutated[fixture.file] = fixture.mutate(mutated[fixture.file]);
    if (!unsignedPackagingFailures(mutated).length) {
      throw new Error(`unsigned-packaging self-test did not reject ${fixture.name}`);
    }
  }
  return fixtures.length;
}

function main() {
  const checks = validateUnsignedPackaging();
  const selfTests = process.argv.includes('--self-test') ? selfTest(process.cwd()) : 0;
  console.log(`unsigned-packaging-contract checks=${checks} selfTests=${selfTests}`);
}

const entry = process.argv[1] ? pathToFileURL(path.resolve(process.argv[1])).href : '';
if (import.meta.url === entry) main();

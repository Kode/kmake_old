import {Exporter} from './exporter';
import {Project} from './project';

export class LinuxExporter extends Exporter {
	constructor() {
		super();
	}

	exportSolution(project: Project): string {
		return this.exportCodeBlocks(project);
	}

	exportCodeBlocks(project: Project): string {
		let units: any[] = [];

		let cbp = {
			CodeBlocks_project_file: {
				FileVersion: {
					major: 1,
					minor: 6
				}
			},
			Project: {
				Option: {
					title: project.name
				},
				Option_: {
					pch_mode: 2
				},
				Option__: {
					compiler: 'gcc'
				},
				Build: {
					Target: {
						title: 'Debug',
						Option: [
							{
								output: 'bin/debug/' + project.name,
								prefix_auto: 1,
								extension_auto: 1
							},
							{
								object_output: 'obj/Debug/'
							},
							{
								type: 1
							},
							{
								compiler: 'gcc'
							},
						],
						Compiler: {
							Add: {
								option: '-g'
							}
						}
					},
					Target_: {
						title: 'Release',
						Option: [
							{
								output: 'bin/Release/' + project.name,
								prefix_auto: 1,
								extension_auto: 1
							},
							{
								object_output: 'obj/Release/'
							},
							{
								type: 0
							},
							{
								compiler: 'gcc'
							},
						],
						Compiler: {
							Add: {
								option: '-O2'
							}
						},
						Linker: {
							Add: {
								option: '-s'
							}
						}
					}
				},
				Compiler: {
					Add: {
						option: '-Wall'
					}
				},
				Linker: {
					Add: [
						{
							option: '-pthread'
						},
						{
							option: '-static-libgcc'
						},
						{
							option: '-static-libstdc++'
						},
						{
							option: '-Wl,-rpath,.'
						}
					]
				},
				Unit: units,
				Extensions: {
					code_completion: {},
					debugger: {}
				}
			}
		}

		for (const file of project.codeFiles) {
			if (file.endsWith('.c') || file.endsWith('.cc') || file.endsWith('.cpp')) {
				units.push(
					{
						filename: file,
						Option: {
							compilerVar: 'CC'
						}
					}
				);
			}
			else if (file.endsWith('.h')) {
				units.push(
					{
						filename: file,
						Option: [
							{
								compile: 1
							},
							{
								weight: 0
							}
						]
					}
				);
			}
		}

		return this.toXml(cbp);
	}

	toXml(obj: any): string {
		return '';
	}
}

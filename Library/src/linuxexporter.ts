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
				Option: [
					{
						title: project.name
					},
					{
						pch_mode: 2
					},
					{
						compiler: 'gcc'
					}
				],
				Build: {
					Target: [
						{
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
						{
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
					]
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
	
	toXmlInner(obj: any): string {
		let xml = '';
		for (const [key, value] of Object.entries(obj)) {
			if (Array.isArray(value)) {
				xml += '<' + key + '>\n';
				for (let i = 0; i < value.length; ++i) {
					xml += this.toXmlInner(value[i]) + '\n';
				}
				xml += '</' + key + '>\n';
			}
			else if (typeof value === 'object' && value !== null) {
				//xml += '\t<' + key + '>\n' + this.toXmlInner(value) + '</' + key + '>\n';
				xml += '<' + key + '></' + key + '>\n';
			}
			else {
				xml += '\t<' + key + '>\n' + value + '</' + key + '>\n';
			}
		}
		return xml;
	}

	toXml(obj: any): string {
		let xml = '<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n';

		for (const [key, value] of Object.entries(obj)) {
			if (typeof value === 'object' && value !== null) {
				xml += '<' + key + '>\n' + this.toXmlInner(value) + '</' + key + '>\n';
			}
			else {
				xml += '<' + key + '>\n' + value + '</' + key + '>\n';
			}
		}

		return xml;
	}
}

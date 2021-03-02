import {Project} from './project';

export abstract class Exporter {
	constructor() {

	}

	exportSolution(project: Project, from: string, to: string, platform: string, vrApi: any, options: any): string {
		return '';
	}
}

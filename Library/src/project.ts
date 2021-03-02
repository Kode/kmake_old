export class Project {
	name: string;
	code: string[];
	shaders: string[];
	assets: string[];

	constructor(name: string) {
		this.name = name;
		this.code = [];
		this.shaders = [];
		this.assets = [];
	}

	addCode(pattern: string) {
		this.code.push(pattern);
	}

	addShaders(pattern: string) {
		this.shaders.push(pattern);
	}

	addAssets(pattern: string) {
		this.assets.push(pattern);
	}

	async addProject(projectPath: string) {
		return new Project('test');
	}

	setDebugDir(dir: string) {

	}
}

export class Project {
    constructor(name) {
        this.name = name;
        this.code = [];
        this.shaders = [];
        this.assets = [];
    }
    addCode(pattern) {
        this.code.push(pattern);
    }
    addShaders(pattern) {
        this.shaders.push(pattern);
    }
    addAssets(pattern) {
        this.assets.push(pattern);
    }
    async addProject(projectPath) {
        return new Project('test');
    }
    setDebugDir(dir) {
    }
}
//# sourceMappingURL=project.js.map
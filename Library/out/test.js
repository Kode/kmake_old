import { Project } from './project';
import { LinuxExporter } from './linuxexporter';
const exporter = new LinuxExporter();
const project = new Project('test');
project.codeFiles = ['a.c', 'b.c', 'c.c'];
console.log(exporter.exportSolution(project));
//# sourceMappingURL=test.js.map
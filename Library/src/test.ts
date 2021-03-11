import { Project } from './project.js';
import { LinuxExporter } from './linuxexporter.js';

const exporter = new LinuxExporter();
const project = new Project('test');
project.codeFiles = ['a.c', 'b.c', 'c.c'];
console.log(exporter.exportSolution(project));

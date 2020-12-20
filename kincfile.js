let project = new Project('kmake');

project.cmd = true;
project.cpp11 = true;
project.linkTimeOptimization = false;
project.addFile('Sources/**');

await project.addProject('Chakra/Build');

project.setDebugDir('Deployment');

resolve(project);

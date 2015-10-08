Module.workerMode = workerMode;

if (workerMode) {

    Module.print = function(msg) {
        self.postMessage({eventType: 'print', text: msg});
    };

    Module.printErr = function(msg) {
        self.postMessage({eventType: 'print', text: msg});
    };
}


Module.noExitRuntime = true;

if (nodejs) {
    Module.preRun = function() {
      FS.mkdir('root');
      FS.mount(NODEFS, { root: '/' }, 'root');
      FS.chdir('root/' + process.cwd());
    }
}

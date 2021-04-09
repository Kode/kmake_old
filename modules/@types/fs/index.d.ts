
declare namespace fs {
	type File = any;

	function open(path:string,flags:string):File;
	function close(file:File):boolean;
	function write(file:File,text:string);
	function writeFile(file:File | string,text:string);
	
	/**
	 *  Reads the entire contents of a file and returns it.
	 * @param filename filename 
	 */
	function readFile(filename:string):string;
	
	/**
	 * 
	 * @param path Path to the directory
	 * @param options
	 * - - `encoding` Default: `"utf8"`. Other possible value is `"buffer"`.
	 * - `withFileTypes`: Default: `false`
	 * @returns String array if encoding is utf8 else array of buffers
	 */
	function readdir(path: string,options?: string | {encoding?:string,withFileTypes?:boolean}):string[] | ArrayBufferLike
	
	/**
	 * Ensures that the directory exists. If the directory structure does not exist, it is created. If provided, options may specify the desired mode for the directory and/or if we should call it recursivaly.
	 * @param path Path to the directory
	 * @param options an integer representing the desired mode or an object specifying if it should be recursive
	 */
	function ensureDir(path: string,options?: number | {recursive?: boolean,mode?: number}):boolean;
	
	/**
	 * 
	 * @param src  Note that if src is a directory it will copy everything inside of this directory, not the entire directory itself
	 * @param dest  Note that if `src` is a file, `dest` cannot be a directory
	 * @param options 
	 * - - `errorOnExist`: when overwrite is false and the destination exists, throw an error. Default is false.
	 * - `overwrite`: overwrite existing file or directory, default is true. Note that the copy operation will silently fail if you set this to false and the destination exists. Use the errorOnExist option to change this behavior.
	 */
	function copy(src: string, dest:string, options?:{overwrite:boolean, errorOnExist:boolean} ):void;

	/**
	 * Test whether or not the given path exists by checking with the file system.
	 * @param path Filesystem path
	 */
	function exists(path: string):boolean;

	/**
	 * 
	 * @param path Path to the directory
	 * @param options 
	 * - - `recursive` Default: false
	 * - `mode`: Not supported on Windows. Default: `0`.
	 */
	function mkdir(path: string, options?:{recursive?:boolean, mode?:number}):boolean;
}
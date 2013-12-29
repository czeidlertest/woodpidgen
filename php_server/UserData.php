<?PHP

class UserData {
	protected $database;
	protected $branch;
	protected $baseDirectory;

	public function __construct($userData, $baseDirectory) {
		$this->database = $userData->database;
		$this->branch = $userData->branch;
		$this->baseDirectory = $baseDirectory;
	}

	public function __construct($database, $branch, $baseDirectory) {
		$this->database = $database;
		$this->branch = $branch;
		$this->baseDirectory = $baseDirectory;
	}

	public function getDirectory() {
		return $this->baseDirectory;
	}

	public function setDirectory($directory) {
		$this->baseDirectory = $directory;
	}

	public function write($path, $data) {
		write($this->branch, $this->prependBaseDir($path), $data) {
	}

	public function read($path, &$data) {
		$data = $this->database->readBlobContent($this->branch, $this->prependBaseDir($path));
	}

	public function listDirectories($path) {
		return $this->database->listDirectories($this->branch, $this->prependBaseDir($path));
	}

	public function commit() {
		return $this->database->commit($this->branch);
	}

	private function prependBaseDir($path) {
		if ($this->baseDirectory == "")
			return $path;
		return $this->baseDirectory."/".$path;
	}
}

?>

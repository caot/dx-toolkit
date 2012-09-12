#include "File.h"

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

#include "api_helper.h"
#include "log.h"

using namespace std;

void testFileExists(const string &filename) {
  LOG << "Testing existence of local file " << filename << "...";
  fs::path p(filename);
  if (fs::exists(p)) {
    LOG << " success." << endl;
  } else {
    LOG << " failure." << endl;
    throw runtime_error("Local file " + filename + " does not exist.");
  }
}

File::File(const string &localFile_, const string &projectSpec_, const string &folder_, const string &name_, const bool toCompress_, const string &mimeType_)
  : localFile(localFile_), projectSpec(projectSpec_), folder(folder_), name(name_), failed(false), waitOnClose(false), closed(false), toCompress(toCompress_), mimeType(mimeType_)  {
  init();
}

void File::init(void) {
  projectID = resolveProject(projectSpec);
//  testProjectPermissions(projectID);
  createFolder(projectID, folder);

  testFileExists(localFile);
  
  string remoteFileName = name;
  if (toCompress) 
    remoteFileName += ".gz";

  fileID = createFileObject(projectID, folder, remoteFileName, mimeType);
  LOG << "fileID is " << fileID << endl;

  cerr << "Uploading file " << localFile << " to file object " << fileID << endl;
}

unsigned int File::createChunks(BlockingQueue<Chunk *> &queue, const int chunkSize, const int tries) {
  LOG << "Creating chunks:" << endl;
  fs::path p(localFile);
  const int64_t size = fs::file_size(p);
  unsigned int numChunks = 0;
  for (int64_t start = 0; start < size; start += chunkSize) {
    int64_t end = min(start + chunkSize, size);
    Chunk * c = new Chunk(localFile, fileID, numChunks, tries, start, end, toCompress);
    c->log("created");
    queue.produce(c);
    ++numChunks;
  }
  return numChunks;
}

void File::close(void) {
  closeFileObject(fileID);
}

void File::updateState(void) {
  string state = getFileState(fileID);
  if (state == "closed") {
    LOG << "File " << fileID << " is closed." << endl;
  }
  closed = (state == "closed");
}

ostream &operator<<(ostream &out, const File &file) {
  out << file.localFile << " (" << file.fileID << ")";
  return out;
}

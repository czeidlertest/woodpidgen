#include "gitinterface.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QTextStream>


class PackManager {
public:
    PackManager(GitInterface *gitInterface, git_repository *repository, git_odb *objectDatabase);

    WP::err exportPack(QByteArray &pack, const QString &commitOldest, const QString &endCommit, int format = -1) const;
    WP::err importPack(const QByteArray &data, const QString &first, const QString &last);

private:
    int readTill(const QByteArray &in, QString &out, int start, char stopChar);

    void findMissingObjects(QList<QString> &listOld, QList<QString> &listNew, QList<QString> &missing) const;
    //! Collect all ancestors including the start $commit.
    WP::err collectAncestorCommits(const QString &commit, QList<QString> &ancestors) const;
    WP::err collectMissingBlobs(const QString &commitStop, const QString &commitLast, QList<QString> &blobs, int type = -1) const;
    WP::err packObjects(const QList<QString> &objects, QByteArray &out) const;
    WP::err listTreeObjects(const git_oid *treeId, QList<QString> &objects) const;

private:
    GitInterface *fDatabase;
    git_repository *fRepository;
    git_odb *fObjectDatabase;
};


QString oidToQString(const git_oid *oid)
{
    char buffer[GIT_OID_HEXSZ+1];
    git_oid_tostr(buffer, GIT_OID_HEXSZ+1, oid);
    return QString(buffer);
}

void oidFromQString(git_oid *oid, const QString &string)
{
    QByteArray data = string.toLatin1();
    git_oid_fromstrn(oid, data.data(), data.count());
}

PackManager::PackManager(GitInterface *gitInterface, git_repository *repository, git_odb *objectDatabase) :
    fDatabase(gitInterface),
    fRepository(repository),
    fObjectDatabase(objectDatabase)
{
}

WP::err PackManager::importPack(const QByteArray& data, const QString &first, const QString &last)
{
    int objectStart = 0;
    while (objectStart < data.length()) {
        QString hash;
        QString size;
        int objectEnd = objectStart;
        objectEnd = readTill(data, hash, objectEnd, ' ');
        objectEnd = readTill(data, size, objectEnd, '\0');
        int blobStart = objectEnd;
        objectEnd += size.toInt();

        const char* dataPointer = data.data() + blobStart;
        WP::err error = fDatabase->writeFile(hash, dataPointer, objectEnd - blobStart);
        if (error != WP::kOk)
            return error;

        objectStart = objectEnd;
    }

    // update tip
    return fDatabase->updateTip(last);
}

int PackManager::readTill(const QByteArray& in, QString &out, int start, char stopChar)
{
    int pos = start;
    while (pos < in.length() && in.at(pos) != stopChar) {
        out.append(in.at(pos));
        pos++;
    }
    pos++;
    return pos;
}

void PackManager::findMissingObjects(QList<QString> &listOld, QList<QString>& listNew, QList<QString> &missing) const
{
    qSort(listOld.begin(), listOld.end());
    qSort(listNew.begin(), listNew.end());

    int a = 0;
    int b = 0;
    while (a < listOld.count() || b < listNew.count()) {
        int cmp;
        if (a < listOld.count() && b < listNew.count())
            cmp = listOld[a].compare(listNew[b]);
        else
            cmp = 0;
        if (b >= listNew.count() || cmp < 0)
            a++;
        else if (a >= listOld.count() || cmp > 0) {
            missing.append(listNew[b]);
            b++;
        } else {
            a++;
            b++;
        }
    }
}

WP::err PackManager::collectAncestorCommits(const QString &commit, QList<QString> &ancestors) const {
    QList<QString> commits;
    commits.append(commit);
    while (commits.count() > 0) {
        QString currentCommit = commits.takeFirst();
        if (ancestors.contains(currentCommit))
            continue;
        ancestors.append(currentCommit);

        // collect parents
        git_commit *commitObject;
        git_oid currentCommitOid;
        oidFromQString(&currentCommitOid, currentCommit);
        if (git_commit_lookup(&commitObject, fRepository, &currentCommitOid) != 0)
            return WP::kError;
        for (unsigned int i = 0; i < git_commit_parentcount(commitObject); i++) {
            const git_oid *parent = git_commit_parent_id(commitObject, i);

            QString parentString = oidToQString(parent);
            commits.append(parentString);
        }
        git_commit_free(commitObject);
    }
    return WP::kOk;
}

WP::err PackManager::collectMissingBlobs(const QString &commitStop, const QString &commitLast, QList<QString> &blobs, int type) const {
    QList<QString> commits;
    QList<QString> newObjects;
    commits.append(commitLast);
    QList<QString> stopAncestorCommits;
    bool stopAncestorsCalculated = false;
    while (commits.count() > 0) {
        QString currentCommit = commits.takeFirst();
        if (currentCommit == commitStop)
            continue;
        if (newObjects.contains(currentCommit))
            continue;
        newObjects.append(currentCommit);

        // collect tree objects
        git_commit *commitObject;
        git_oid currentCommitOid;
        oidFromQString(&currentCommitOid, currentCommit);
        if (git_commit_lookup(&commitObject, fRepository, &currentCommitOid) != 0)
            return WP::kError;
        WP::err status = listTreeObjects(git_commit_tree_id(commitObject), newObjects);
        if (status != WP::kOk) {
            git_commit_free(commitObject);
            return WP::kError;
        }

        // collect parents
        unsigned int parentCount = git_commit_parentcount(commitObject);
        if (parentCount > 1 && !stopAncestorsCalculated) {
            collectAncestorCommits(commitStop, stopAncestorCommits);
            stopAncestorsCalculated = true;
        }
        for (unsigned int i = 0; i < parentCount; i++) {
            const git_oid *parent = git_commit_parent_id(commitObject, i);

            QString parentString = oidToQString(parent);
            // if we reachted the ancestor commits tree we are done
            if (!stopAncestorCommits.contains(parentString))
                commits.append(parentString);
        }
        git_commit_free(commitObject);
    }

    // get stop commit object tree
    QList<QString> stopCommitObjects;
    if (commitStop != "") {
        git_commit *stopCommitObject;
        git_oid stopCommitOid;
        oidFromQString(&stopCommitOid, commitStop);
        if (git_commit_lookup(&stopCommitObject, fRepository, &stopCommitOid) != 0)
            return WP::kError;
        WP::err status = listTreeObjects(git_commit_tree_id(stopCommitObject), stopCommitObjects);
        git_commit_free(stopCommitObject);
        if (status != WP::kOk)
            return WP::kError;
    }

    // calculate the missing objects
    findMissingObjects(stopCommitObjects, newObjects, blobs);
    return WP::kOk;

/*
    QList<QString> handledCommits;
    QList<QString> commits;
    commits.append(commitLast);
    while (true) {
        if (commits.count() == 0)
            break;
        QString currentCommit = commits.takeLast();
        if (currentCommit == commitStop && handledCommits.contains(currentCommit))
            continue;
        if (handledCommits.contains(currentCommit))
            continue;
        handledCommits.append(currentCommit);

        git_commit *commitObject;
        git_oid currentCommitOid;
        oidFromQString(&currentCommitOid, currentCommit);
        if (git_commit_lookup(&commitObject, fRepository, &currentCommitOid) != 0)
            return WP::kError;
        for (unsigned int i = 0; i < git_commit_parentcount(commitObject); i++) {
            const git_oid *parent = git_commit_parent_id(commitObject, i);

            git_commit *parentObject;
            if (git_commit_lookup(&parentObject, fRepository, parent) != 0) {
                git_commit_free(commitObject);
                return WP::kError;
            }

            QList<QString> diffs;
            QList<QString> parentObjects;
            WP::err status = listTreeOjects(git_commit_tree_id(parentObject), parentObjects);
            git_commit_free(parentObject);
            if (status != WP::kOk) {
                git_commit_free(commitObject);
                return WP::kError;
            }
            QList<QString> commitObjects;
            status = listTreeOjects(git_commit_tree_id(commitObject), commitObjects);
            if (status != WP::kOk) {
                git_commit_free(commitObject);
                return WP::kError;
            }
            findMissingObjects(parentObjects, commitObjects, diffs);

            for (int i = 0; i < diffs.count(); i++) {
                git_object *object;
                git_oid objectOid;
                git_oid_fromstrn(&objectOid, diffs[i].toLatin1().data(), sizeof(objectOid.id));
                if (git_object_lookup(&object, fRepository, &objectOid, GIT_OBJ_ANY) != 0) {
                    git_commit_free(commitObject);
                    return WP::kError;
                }
                if (type <= 0 || git_object_type(object) == type) {
                    if (!blobs.contains(diffs[i]))
                        blobs.append(diffs[i]);

                }
                git_object_free(object);
            }

            commits.append(oidToQString(parent));
        }
        git_commit_free(commitObject);
    }

    blobs.append(handledCommits);
    return WP::kOk;*/
}

#include "zlib.h"
#include <QBuffer>
#define CHUNK 16384

int gzcompress(QByteArray &source, QByteArray &outArray)
{
    QBuffer sourceBuffer(&source);
    sourceBuffer.open(QIODevice::ReadOnly);
    QBuffer outBuffer(&outArray);
    outBuffer.open(QIODevice::WriteOnly);

    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    int status = deflateInit(&stream, Z_DEFAULT_COMPRESSION);
    if (status != Z_OK)
        return status;

    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
    int flush = Z_NO_FLUSH ;
    while (flush != Z_FINISH) {
        stream.avail_in = sourceBuffer.read((char*)in, CHUNK);
        flush = sourceBuffer.atEnd() ? Z_FINISH : Z_NO_FLUSH;
        stream.next_in = in;

        unsigned have;
        // deflate() till output buffer is full or we are done
        do {
            stream.avail_out = CHUNK;
            stream.next_out = out;
            status = deflate(&stream, flush); // no bad return value
            have = CHUNK - stream.avail_out;
            if (outBuffer.write((char*)out, have) != have) {
                (void)deflateEnd(&stream);
                return Z_ERRNO;
            }
        } while (stream.avail_out == 0);
    }

    deflateEnd(&stream);
    return Z_OK;
}

WP::err PackManager::packObjects(const QList<QString> &objects, QByteArray &out) const
{
    for (int i = 0; i < objects.count(); i++) {
        git_odb_object *object;
        git_oid objectOid;
        oidFromQString(&objectOid, objects.at(i));
        if (git_odb_read(&object, fObjectDatabase, &objectOid) != 0)
            return WP::kError;

        QByteArray blob;
        blob.append(git_object_type2string(git_odb_object_type(object)));
        blob.append(' ');
        QString sizeString;
        QTextStream(&sizeString) << git_odb_object_size(object);
        blob.append(sizeString);
        blob.append('\0');
        blob.append((char*)git_odb_object_data(object), git_odb_object_size(object));
        //blob = qCompress(blob);

        QByteArray blobCompressed;
        int error = gzcompress(blob, blobCompressed);
        if (error != Z_OK)
            return WP::kError;

        out.append(objects[i]);
        out.append(' ');
        QString blobSize;
        QTextStream(&blobSize) << blobCompressed.count();
        out.append(blobSize);
        out.append('\0');
        out.append(blobCompressed);
    }

    return WP::kOk;
}

WP::err PackManager::exportPack(QByteArray &pack, const QString &commitOldest, const QString &commitLatest, int format) const
{
    QString commitEnd(commitLatest);
    if (commitLatest == "")
        commitEnd = fDatabase->getTip();
    if (commitEnd == "")
        return WP::kNotInit;

    QList<QString> blobs;
    collectMissingBlobs(commitOldest, commitEnd, blobs);
    return packObjects(blobs, pack);
}

WP::err PackManager::listTreeObjects(const git_oid *treeId, QList<QString> &objects) const
{
    git_tree *tree;
    int error = git_tree_lookup(&tree, fRepository, treeId);
    if (error != 0)
        return WP::kError;
    QString treeOidString = oidToQString(treeId);
    if (!objects.contains(treeOidString))
        objects.append(treeOidString);
    QList<git_tree*> treesQueue;
    treesQueue.append(tree);

    while (error == 0) {
        if (treesQueue.count() < 1)
            break;
        git_tree *currentTree = treesQueue.takeFirst();

        for (unsigned int i = 0; i < git_tree_entrycount(currentTree); i++) {
            const git_tree_entry *entry = git_tree_entry_byindex(currentTree, i);
            QString objectOidString = oidToQString(git_tree_entry_id(entry));
            if (!objects.contains(objectOidString))
                objects.append(objectOidString);
            if (git_tree_entry_type(entry) == GIT_OBJ_TREE) {
                git_tree *subTree;
                error = git_tree_lookup(&subTree, fRepository, git_tree_entry_id(entry));
                if (error != 0)
                    break;
                treesQueue.append(subTree);
            }
        }
        git_tree_free(currentTree);
    }
    if (error != 0) {
        for (int i = 0; i < treesQueue.count(); i++)
            git_tree_free(treesQueue[i]);
        return WP::kError;
    }
    return WP::kOk;
}


GitInterface::GitInterface()
    :
    fRepository(NULL),
    fObjectDatabase(NULL),
    fCurrentBranch("master")
{
    fNewRootTreeOid.id[0] = '\0';
}


GitInterface::~GitInterface()
{
    unSet();
}

WP::err GitInterface::setTo(const QString &path, bool create)
{
    unSet();
    fRepositoryPath = path;

    int error = git_repository_open(&fRepository, fRepositoryPath.toLatin1().data());

    if (error != 0 && create)
        error = git_repository_init(&fRepository, fRepositoryPath.toLatin1().data(), true);

    if (error != 0)
        return (WP::err)error;

    return (WP::err)git_repository_odb(&fObjectDatabase, fRepository);
}

void GitInterface::unSet()
{
    git_repository_free(fRepository);
    git_odb_free(fObjectDatabase);
}

QString GitInterface::path()
{
    return fRepositoryPath;
}

WP::err GitInterface::setBranch(const QString &branch, bool /*createBranch*/)
{
    if (fRepository == NULL)
        return WP::kNotInit;
    fCurrentBranch = branch;
    return WP::kOk;
}

QString GitInterface::branch() const
{
    return fCurrentBranch;
}

WP::err GitInterface::write(const QString& path, const QByteArray &data)
{
    QString treePath = path;
    QString filename = removeFilename(treePath);

    git_oid oid;
    int error = git_odb_write(&oid, fObjectDatabase, data.data(), data.count(), GIT_OBJ_BLOB);
    if (error != WP::kOk)
        return (WP::err)error;
    git_filemode_t fileMode = GIT_FILEMODE_BLOB;

    git_tree *rootTree = NULL;
    if (fNewRootTreeOid.id[0] != '\0') {
        error = git_tree_lookup(&rootTree, fRepository, &fNewRootTreeOid);
        if (error != 0)
            return WP::kError;
    } else
        rootTree = getTipTree();

    while (true) {
        git_tree *node = NULL;
        if (rootTree == NULL || treePath == "")
            node = rootTree;
        else {
            git_tree_entry *treeEntry;
            error = git_tree_entry_bypath(&treeEntry, rootTree, treePath.toLatin1().data());
            if (error == 0) {
                error = git_tree_lookup(&node, fRepository, git_tree_entry_id(treeEntry));
                git_tree_entry_free(treeEntry);
                if (error != 0)
                    return WP::kError;
            }
        }

        git_treebuilder *builder = NULL;
        error = (WP::err)git_treebuilder_create(&builder, node);
        if (node != rootTree)
            git_tree_free(node);
        if (error != WP::kOk) {
            git_tree_free(rootTree);
            return (WP::err)error;
        }
        error = git_treebuilder_insert(NULL, builder, filename.toLatin1().data(), &oid, fileMode);
        if (error != WP::kOk) {
            git_tree_free(rootTree);
            git_treebuilder_free(builder);
            return (WP::err)error;
        }
        error = git_treebuilder_write(&oid, fRepository, builder);
        git_treebuilder_free(builder);
        if (error != WP::kOk) {
            git_tree_free(rootTree);
            return (WP::err)error;
        }

        // in the folloing we write trees
        fileMode = GIT_FILEMODE_TREE;
        if (treePath == "")
            break;
        filename = removeFilename(treePath);
    }

    git_tree_free(rootTree);
    fNewRootTreeOid = oid;
    return WP::kOk;
}

WP::err GitInterface::remove(const QString &path)
{
    // TODO implement
    return WP::kError;
}

WP::err GitInterface::commit()
{
    git_tree *tree;
    int error = git_tree_lookup(&tree, fRepository, &fNewRootTreeOid);
    if (error != 0)
        return (WP::err)error;

    QString oldCommit = getTip();

    QString refName = "refs/heads/";
    refName += fCurrentBranch;

    git_signature* signature;
    git_signature_new(&signature, "client", "no mail", time(NULL), 0);

    git_commit *tipCommit = getTipCommit();
    git_commit *parents[1];
    int nParents = 0;
    if (tipCommit) {
        parents[0] = tipCommit;
        nParents++;
    }

    git_oid id;
    error = git_commit_create(&id, fRepository, refName.toLatin1().data(), signature, signature,
                              NULL, "message", tree, nParents, (const git_commit**)parents);

    git_commit_free(tipCommit);
    git_signature_free(signature);
    git_tree_free(tree);
    if (error != 0)
        return (WP::err)error;

    fNewRootTreeOid.id[0] = '\0';

    emit newCommits(oldCommit, getTip());
    return WP::kOk;

   //git_reference* out;
    //int result = git_reference_lookup(&out, fRepository, "refs/heads/master");
    //git_reference_free(out);

    /*int error;
    git_revwalk *walk;
    error = git_revwalk_new(&walk, fRepository);
    error = git_revwalk_push_glob(walk, "refs/heads/master");
    git_oid oid;
    error = git_revwalk_next(&oid, walk);
    git_commit *commit;
    error = git_commit_lookup(&commit, fRepository, &oid);
    printf("Name %s\n", git_commit_author(commit)->name);
    git_commit_free(commit);
    git_revwalk_free(walk);
    */

    /*
    git_oid id;
    git_oid_fromstr(&id, "229841ec03381379237658d8136a8342b7cd7c23");
    git_reference *newRef;
    int status = git_reference_create_oid(&newRef, fRepository, "refs/heads/test", &id, false);

    git_strarray ref_list;
     git_reference_list(&ref_list, fRepository, GIT_REF_LISTALL);

      const char *refname;
      git_reference *ref;
      char out[41];
      for (int i = 0; i < ref_list.count; ++i) {
          refname = ref_list.strings[i];
          git_reference_lookup(&ref, fRepository, refname);
          if (ref == NULL)
              continue;
    switch (git_reference_type(ref)) {
        case GIT_REF_OID:
          git_oid_fmt(out, git_reference_oid(ref));
          out[40] = '\0';
          printf("%s [%s]\n", refname, out);
          break;

        case GIT_REF_SYMBOLIC:
          printf("%s => %s\n", refname, git_reference_target(ref));
          break;
        default:
          fprintf(stderr, "Unexpected reference type\n");
          exit(1);
      }
      }

      git_strarray_free(&ref_list);*/

}

WP::err GitInterface::read(const QString &path, QByteArray &data) const
{
    QString pathCopy = path;
    while (!pathCopy.isEmpty() && pathCopy.at(0) == '/')
        pathCopy.remove(0, 1);

    git_tree *rootTree = NULL;
    if (fNewRootTreeOid.id[0] != '\0') {
        int error = git_tree_lookup(&rootTree, fRepository, &fNewRootTreeOid);
        if (error != 0)
            return WP::kError;
    } else
        rootTree = getTipTree();

    if (rootTree == NULL)
        return WP::kNotInit;

    git_tree_entry *treeEntry;
    int error = git_tree_entry_bypath(&treeEntry, rootTree, pathCopy.toLatin1().data());
    if (error != 0)
        return WP::kError;

    git_blob *blob;
    error = git_blob_lookup(&blob, fRepository, git_tree_entry_id(treeEntry));
    git_tree_entry_free(treeEntry);
    if (error != 0)
        return WP::kEntryNotFound;

    data.clear();
    data.append((const char*)git_blob_rawcontent(blob), git_blob_rawsize(blob));

    git_blob_free(blob);
    return WP::kOk;
}

WP::err GitInterface::writeObject(const char *data, int size)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(data, size);
    QByteArray hashBin =  hash.result();
    QString hashHex =  hashBin.toHex();

    QByteArray arrayData;
    arrayData = QByteArray::fromRawData(data, size);
    QByteArray compressedData = qCompress(arrayData);

    return writeFile(hashHex, compressedData.data(), compressedData.size());
}

WP::err GitInterface::writeFile(const QString &hash, const char *data, int size)
{
    std::string hashHex = hash.toStdString();
    QString path;
    path.sprintf("%s/objects", fRepositoryPath.toStdString().c_str());
    QDir dir(path);
    dir.mkdir(hashHex.substr(0, 2).c_str());
    path += "/";
    path += hashHex.substr(0, 2).c_str();
    path += "/";
    path += hashHex.substr(2).c_str();
    QFile file(path);
    if (file.exists())
        return WP::kOk;

    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    if (file.write(data, size) < 0)
        return WP::kError;
    return WP::kOk;
}

QString GitInterface::getTip() const
{
    QString foundOid = "";
    QString refName = "refs/heads/";
    refName += fCurrentBranch;

    git_strarray ref_list;
    git_reference_list(&ref_list, fRepository);

    for (unsigned int i = 0; i < ref_list.count; ++i) {
        const char *name = ref_list.strings[i];
        if (refName != name)
            continue;

        git_oid out;
        int error = git_reference_name_to_id(&out, fRepository, name);
        if (error != 0)
            break;
        char buffer[41];
        git_oid_fmt(buffer, &out);
        buffer[40] = '\0';
        foundOid = buffer;
        break;
    }

    git_strarray_free(&ref_list);

    return foundOid;

/*
    QString refPath = fRepositoryPath;
    refPath += "/refs/heads/";
    refPath += branch;

    QFile file(refPath);
    if (!file.open(QIODevice::ReadOnly))
        return "";
    QString oid;
    QTextStream outstream(&file);
    outstream >> oid;
    return oid;*/
}


WP::err GitInterface::updateTip(const QString &commit)
{
    QString refPath = "refs/heads/";
    refPath += fCurrentBranch;
    git_oid id;
    git_oid_fromstr(&id, commit.toLatin1().data());
    git_reference *newRef;
    int status = git_reference_create(&newRef, fRepository, refPath.toStdString().c_str(), &id, true);
    if (status != 0)
        return WP::kError;
    return WP::kOk;

   /* QString refPath = fRepositoryPath;
    refPath += "/refs/heads/";
    refPath += branch;

    QFile file(refPath);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.resize(0);
    QTextStream outstream(&file);
    outstream << last << "\n";

    return true;*/
}

QStringList GitInterface::listFiles(const QString &path) const
{
   return listDirectoryContent(path, GIT_OBJ_BLOB);
}

QStringList GitInterface::listDirectories(const QString &path) const
{
    return listDirectoryContent(path, GIT_OBJ_TREE);
}

WP::err GitInterface::exportPack(QByteArray &pack, const QString &startCommit, const QString &endCommit, int format) const
{
    PackManager packManager((GitInterface*)this, fRepository, fObjectDatabase);
    return packManager.exportPack(pack, startCommit, endCommit, format);
}

WP::err GitInterface::importPack(const QByteArray &pack, const QString &startCommit, const QString &endCommit, int format)
{
    PackManager packManager(this, fRepository, fObjectDatabase);
    WP::err error = packManager.importPack(pack, startCommit, endCommit);
    if (error == WP::kOk)
        emit newCommits(startCommit, endCommit);
    return error;
}

QStringList GitInterface::listDirectoryContent(const QString &path, int type) const
{
    QStringList list;
    git_tree *tree = getDirectoryTree(path);
    if (tree == NULL)
        return list;
    int count = git_tree_entrycount(tree);
    for (int i = 0; i < count; i++) {
        const git_tree_entry *entry = git_tree_entry_byindex(tree, i);
        if (type != -1 && git_tree_entry_type(entry) != type)
            continue;
        list.append(git_tree_entry_name(entry));
    }
    git_tree_free(tree);
    return list;
}

git_commit *GitInterface::getTipCommit() const
{
    QString tip = getTip();
    git_oid out;
    int error = git_oid_fromstr(&out, tip.toStdString().c_str());
    if (error != 0)
        return NULL;
    git_commit *commit;
    error = git_commit_lookup(&commit, fRepository, &out);
    if (error != 0)
        return NULL;
    return commit;
}

git_tree *GitInterface::getTipTree() const
{
    git_tree *rootTree = NULL;
    git_commit *commit = getTipCommit();
    if (commit != NULL) {
        int error = git_commit_tree(&rootTree, commit);
        git_commit_free(commit);
        if (error != 0) {
            printf("can't get tree from commit\n");
            return NULL;
        }
    }
    return rootTree;
}

git_tree *GitInterface::getDirectoryTree(const QString &dirPath) const
{
    git_tree *tree = getTipTree();
    if (tree == NULL)
        return tree;

    QString dir = dirPath.trimmed();
    while (!dir.isEmpty() && dir.at(0) == '/')
        dir.remove(0, 1);
    while (!dir.isEmpty() && dir.at(dir.count() - 1) == '/')
        dir.remove(dir.count() - 1, 1);
    if (dir.isEmpty())
        return tree;

    while (!dir.isEmpty()) {
        int slash = dir.indexOf("/");
        QString subDir;
        if (slash < 0) {
            subDir = dir;
            dir = "";
        } else {
            subDir = dir.left(slash);
            dir.remove(0, slash + 1);
        }
        const git_tree_entry *entry = git_tree_entry_byname(tree, subDir.toLatin1().data());
        if (entry == NULL || git_tree_entry_type(entry) != GIT_OBJ_TREE) {
            git_tree_free(tree);
            return NULL;
        }
        const git_oid *oid = git_tree_entry_id(entry);
        git_tree *newTree = NULL;
        int error = git_tree_lookup(&newTree, fRepository, oid);
        if (error != 0) {
            git_tree_free(tree);
            return NULL;
        }
        tree = newTree;
    }
    return tree;
}

QString GitInterface::removeFilename(QString &path) const
{
    path = path.trimmed();
    while (!path.isEmpty() && path.at(0) == '/')
        path.remove(0, 1);
    if (path.isEmpty())
        return "";
    while (true) {
        int index = path.lastIndexOf("/", -1);
        if (index < 0) {
            QString filename = path;
            path.clear();
            return filename;
        }

        // ignore multiple slashes
        if (index == path.count() - 1) {
            path.truncate(path.count() -1);
            continue;
        }
        int filenameLength = path.count() - 1 - index;
        QString filename = path.mid(index + 1, filenameLength);
        path.truncate(index);
        while (!path.isEmpty() && path.at(path.count() - 1) == '/')
            path.remove(path.count() - 1, 1);
        return filename;
    }
    return path;
}

#include "gitinterface.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QTextStream>

#include <git2/refs.h>


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

int GitInterface::setBranch(const QString &branch, bool /*createBranch*/)
{
    if (fRepository == NULL)
        return DatabaseInterface::kNotInit;
    fCurrentBranch = branch;
    return 0;
}


int GitInterface::add(const QString& path, const QByteArray &data)
{
    QString treePath = path;
    QString filename = removeFilename(treePath);

    git_oid oid;
    int error = git_odb_write(&oid, fObjectDatabase, data.data(), data.count(), GIT_OBJ_BLOB);
    if (error != 0)
        return error;
    int fileMode = 100644;

    git_tree *rootTree = NULL;
    if (fNewRootTreeOid.id[0] != '\0') {
        error = git_tree_lookup(&rootTree, fRepository, &fNewRootTreeOid);
        if (error != 0)
            return error;
    } else
        rootTree = getTipTree(fCurrentBranch);

    while (true) {
        git_tree *node = NULL;
        if (rootTree == NULL || treePath == "")
            node = rootTree;
        else {
            QString nodeChild = treePath + "/" + filename;
            git_tree_get_subtree(&node, rootTree, nodeChild.toStdString().c_str());
        }

        git_treebuilder *builder = NULL;
        error = git_treebuilder_create(&builder, node);
        if (node != rootTree)
            git_tree_free(node);
        if (error != 0) {
            git_tree_free(rootTree);
            return error;
        }
        error = git_treebuilder_insert(NULL, builder, filename.toStdString().c_str(), &oid, fileMode);
        if (error != 0) {
            git_tree_free(rootTree);
            git_treebuilder_free(builder);
            return error;
        }
        error = git_treebuilder_write(&oid, fRepository, builder);
        git_treebuilder_free(builder);
        if (error != 0) {
            git_tree_free(rootTree);
            return error;
        }

        // in the folloing we write trees
        fileMode = 040000;
        if (treePath == "")
            break;
        filename = removeFilename(treePath);
    }

    git_tree_free(rootTree);
    fNewRootTreeOid = oid;
    return 0;
}

int GitInterface::remove(const QString &path)
{
    // TODO implement
    return 0;
}

int GitInterface::commit()
{
    git_tree *tree;
    int error = git_tree_lookup(&tree, fRepository, &fNewRootTreeOid);
    if (error != 0)
        return error;

    QString refName = "refs/heads/";
    refName += fCurrentBranch;

    git_signature* signature;
    git_signature_new(&signature, "client", "no mail", time(NULL), 0);

    git_commit *tipCommit = getTipCommit(fCurrentBranch);
    git_commit *parents[1];
    int nParents = 0;
    if (tipCommit) {
        parents[0] = tipCommit;
        nParents++;
    }

    git_oid id;
    error = git_commit_create(&id, fRepository, refName.toStdString().c_str(), signature, signature, NULL, "message", tree, nParents, (const git_commit**)parents);

    git_commit_free(tipCommit);
    git_signature_free(signature);
    git_tree_free(tree);
    if (error != 0)
        return error;

    fNewRootTreeOid.id[0] = '\0';
    return 0;
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

int GitInterface::get(const QString &path, QByteArray &data) const
{
    QString pathCopy = path;
    while (!pathCopy.isEmpty() && pathCopy.at(0) == '/')
        pathCopy.remove(0, 1);

    git_tree *rootTree = getTipTree(fCurrentBranch);

    git_tree *node = NULL;
    int error = git_tree_get_subtree(&node, rootTree, pathCopy.toStdString().c_str());
    git_tree_free(rootTree);
    if (error != 0)
        return error;

    QString filename = removeFilename(pathCopy);

    const git_tree_entry *treeEntry = git_tree_entry_byname(node, filename.toStdString().c_str());

    if (treeEntry == NULL) {
        git_tree_free(node);
        return -1;
    }

    git_blob *blob;
    error = git_blob_lookup(&blob, fRepository, git_tree_entry_id(treeEntry));
    git_tree_free(node);
    if (error != 0)
        return error;

    data.clear();
    data.append((const char*)git_blob_rawcontent(blob), git_blob_rawsize(blob));

    git_blob_free(blob);
    return 0;
}

int GitInterface::setTo(const QString &path, bool create)
{
    unSet();
    fRepositoryPath = path;

    int error = git_repository_open(&fRepository, fRepositoryPath.toStdString().c_str());

    if (error != 0 && create)
        error = git_repository_init(&fRepository, fRepositoryPath.toStdString().c_str(), true);

    if (error != 0)
        return error;

    return git_repository_odb(&fObjectDatabase, fRepository);
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

int GitInterface::writeObject(const char *data, int size)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(data, size);
    QByteArray hashBin =  hash.result();
    std::string hashHex =  hashBin.toHex().data();
    printf("hash %s\n", hashHex.c_str());

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
        return -1;

    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    QByteArray arrayData;
    arrayData = QByteArray::fromRawData(data, size);
    QByteArray compressedData = qCompress(arrayData);
    file.write(compressedData.data(), compressedData.size());
    return 0;
}

int GitInterface::writeFile(const QString &hash, const char *data, int size)
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
        return -1;

    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    file.write(data, size);
    return 0;
}

QString GitInterface::getTip(const QString &branch) const
{
    QString foundOid = "";
    QString refName = "refs/heads/";
    refName += branch;

    git_strarray ref_list;
    git_reference_list(&ref_list, fRepository, GIT_REF_LISTALL);

    git_reference *ref;
    char out[41];
    for (unsigned int i = 0; i < ref_list.count; ++i) {
        const char *name = ref_list.strings[i];
        if (refName != name)
            continue;
        git_reference_lookup(&ref, fRepository, name);
        if (ref == NULL)
            continue;
        switch (git_reference_type(ref)) {
        case GIT_REF_OID:
          git_oid_fmt(out, git_reference_oid(ref));
          out[40] = '\0';
          foundOid = out;
          break;

        case GIT_REF_SYMBOLIC:
            foundOid = git_reference_target(ref);
            break;
        default:
            fprintf(stderr, "Unexpected reference type\n");
            foundOid = "";
        }
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


bool GitInterface::updateTip(const QString &branch, const QString &last)
{
    QString refPath = fRepositoryPath;
    refPath += "/refs/heads/";
    refPath += branch;
    git_oid id;
    git_oid_fromstr(&id, last.toStdString().c_str());
    git_reference *newRef;
    int status = git_reference_create_oid(&newRef, fRepository, refPath.toStdString().c_str(), &id, true);
    if (status == 0)
        return true;
    return false;

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

QStringList GitInterface::listDirectoryContent(const QString &path, int type) const
{
    QStringList list;
    git_tree *tree = getDirectoryTree(path, fCurrentBranch);
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

git_commit *GitInterface::getTipCommit(const QString &branch) const
{
    QString tip = getTip(branch);
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

git_tree *GitInterface::getTipTree(const QString &branch) const
{
    git_tree *rootTree = NULL;
    git_commit *commit = getTipCommit(branch);
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

git_tree *GitInterface::getDirectoryTree(const QString &dirPath, const QString &branch) const
{
    git_tree *tree = getTipTree(branch);
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
        const git_tree_entry *entry = git_tree_entry_byname(tree, subDir.toAscii());
        if (git_tree_entry_type(entry) != GIT_OBJ_TREE) {
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

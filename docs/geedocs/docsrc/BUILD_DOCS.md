# Build Instructions for Open GEE Documentation

Open GEE documentation at release 5.3.2 has migrated from HTML format to RST
(reStructuredText) format. To update this documentation, make changes to the RST
source files and then generate HTML using Sphinx.

## Prerequisites

To build the documentation, you need to install Sphinx:

```bash
sudo apt install python-pip
sudo pip install sphinx==1.7.5
sudo pip install GitPython
sudo pip install sphinxprettysearchresults
```

## Documentation Organization

* _earthenterprise/docs/geedocs/docsrc_ contains _index.rst_, which includes
   all the index files for the major topics.

* _docsrc/answer_ has the index files for all the topic folders.

* Each major topic's docs are in a folder under _docsrc/answer_.

For example, _docsrc/index.rst_ includes _fusionTutorial.rst_ (the index file of
the _fusionTutorial_ folder), which includes all the RST files in the
_fusionTutorial_ folder.

## Updating Documentation

1. If you are modifying an existing file, make the changes using RST
   syntax and generate the HTML (step 3).

2. If you are adding a new file under an existing major topic, create an RST
   file under the topic folder and add that file in _answer/topicName.rst_
   where it fits in the topic order. Make the changes using RST syntax and
   generate the HTML (step 3).

3. Generate the HTML:

   ```bash
   cd earthenterprise/docs/geedocs/docsrc
   make html
   ```

   **NOTE**: To Generate HTML in Windows
   ```
   add this in your git config of your repo
   cd earthenterprise
   Add this line in .git/config under [core]
    autocrlf = input
   cd earthenterprise\docs\geedocs\docsrc
   make html
   ```


   Depending on your version of Open GEE, appropriate HTML folders will be
   updated when you execute `make`.

   For example, after running `make` for version 5.3.2, you will see a lot of
   generated files under _earthenterprise/docs/geedocs/5.3.2_.

   Add the RST files that you added or changed under _docsrc_ and the generated
   files under the release folder to your commit.

   ```bash
   git add earthenterprise/docs/geedocs/5.3.2/*
   git add earthenterprise/docs/geedocs/docsrc/*
   ```

    Commit your changes and create a pull request.

## Merging Your Changes with Master

Assume you have branched to release and your release branch is release_5.3.2
and you are making changes that need to go in to the release_5.3.2
branch and master.

1. Create a branch off release_5.3.2 branch

   ```bash
   git checkout -b release_5.3.2 upstream/release_5.3.2
   git checkout -b changes_to_5.3.2
   ```

2. Make your changes as in step 1 or step 2 above. Perform step 3 to generate
   the HTML. This step will update _earthenterprise/docs/geedocs/5.3.2_.

3. Commit RST and 5.3.2 HTML changes and create a pull request to push to
   release_5.3.2 branch.

4. After your changes are merged in to release_5.3.2 branch, merge the
   documentation changes to master:

   a. Update master in your fork:

      ```bash
      git checkout master
      git fetch upstream
      git pull upstream master ( to get the latest changes from upstream)
      git push origin master
      ```

   b. Create a branch off master:

      ```bash
      git checkout master
      git checkout -b merge_5.3.2_to_master
      ```

   c. Pull release_5.3.2 changes:

      ```bash
      $ git pull upstream release_5.3.2
      From git://github.com/google/earthenterprise
      * branch            release_5.3.2 -> FETCH_HEAD
      Auto-merging docs/geedocs/docsrc/index.rst
      CONFLICT (content): Merge conflict in docs/geedocs/docsrc/index.rst
      Automatic merge failed; fix conflicts and then commit the result.
      ```

      There was a merge conflict in _index.rst_.

      ```bash
      <<<<<<< HEAD
      Google Earth Enterprise Documentation Version |release|
      =======================================================

         This documentation contains legacy information that may not apply
         to the Google Earth Enterprise Open Source version. This bundle is
         also available in the GEE Server |release| installation package.
      =======
      Google Earth Enterprise Documentation Version 5.3.3
      ===================================================

         This documentation contains legacy information that may not apply
         to the Google Earth Enterprise Open Source version. This bundle is
         also available in the GEE Server 5.3.3 installation package.
      >>>>>>> 21627632956e75c4255d0701127556cbd46e6b31
      ```

      Keep the change, which has |release| instead of 5.3.3. This will help
      update the documentation based on the current Open GEE release.

      Merges may become tricky if there are conflicts in the generated HTML.
      If bringing 5.3.2 changes into master there will be 5.3.2
      HTML changes. You can keep changes in the 5.3.2 branch while resolving
      conflicts. Resolve conflicts in the RST and generated HTML for master
      which will update the HTML for 5.3.3.

   d. Commit 5.3.2 merge:

      After resolving the merge conflict:

      ```bash
      git add index.rst (add the fixed conflict to the commit)
      git commit
      ```

      Now you have committed the RST changes made in 5.3.2 release and the
      generated 5.3.2 HTML folders.

   e. Generate HTML for 5.3.3 (master):

    ```bash
    cd earthenterprise/docs/geedocs/docsrc
    rm -rf build
    make html
    ```

    Based on the current Open GEE version (5.3.3), it will generate 5.3.3 HTML
    documentation under _earthenterprise/docs/geedocs/5.3.3_.

    Add those files to the commit:

    ```bash
    git add earthenterprise/docs/geedocs/5.3.3/*
    git commit
    ```

   f. Now you can push and create a pull request for these two commits.

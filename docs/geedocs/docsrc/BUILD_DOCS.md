# Build Instructions for Open GEE Documentation

Open GEE documentation at release 5.3.2 has been migrated from HTML format to RST
(reStructuredText) format. To update Open GEE documentation, make changes to the RST
source files and then generate HTML using Sphinx.

## Prerequisites

To build the documentation, the following packages (and versions) need to be installed :

    python 3
    git 1.8+
    python3-pip
    python 3 packages: sphinx(1.7.5), GitPython(1.0.1), gitdb2(<3.0.2), sphinxprettysearchresults(0.3.5)

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
   Open a cmd prompt
   cd earthenterprise
   Add this line in .git/config of your repo under [core]
    autocrlf = input
   cd earthenterprise\docs\geedocs\docsrc
   make html
   ```


   Depending on your version of Open GEE, appropriate HTML folders will be
   updated when you execute `make`.

   For example, after running `make` for version 5.x.y, you will see a lot of
   generated files under _earthenterprise/docs/geedocs/5.x.y_.

   Add the RST files that you added or changed under _docsrc_ and the generated
   files under the release folder to your commit.

   ```bash
   git add earthenterprise/docs/geedocs/5.x.y/*
   git add earthenterprise/docs/geedocs/docsrc/*
   ```

    Commit your changes and create a pull request.

## Making doc Changes in a release folder and merging into Master

1. Create a branch off release_5.x.y branch in your fork

   ```bash
   git checkout -b release_5.x.y upstream/release_5.x.y
   git checkout -b changes_to_5.x.y
   ```

2. Make your changes as in step 1 or step 2 above. Perform step 3 to generate
   the HTML. This step will update _earthenterprise/docs/geedocs/5.x.y_.

3. Commit RST and 5.x.y HTML changes and create a pull request to push to
   release_5.x.y branch.

4. After your changes are merged in to release_5.x.y branch, merge the
   documentation changes to master:

   a. Update master in your fork:

      ```bash
      git checkout master
      git fetch upstream
      git pull upstream master ( to get the latest changes from upstream)
      git push origin master
      ```

   b. Create a branch off master in your fork:

      ```bash
      git checkout master
      git checkout -b merge_5.x.y_to_master
      ```

   c. Pull release_5.x.y changes:

      ```bash
      $ git pull upstream release_5.x.y
      ```

      NOTE: Merges may become tricky if there are conflicts in the generated HTML.
      If bringing 5.x.y changes into master there will be 5.x.y
      HTML changes. You can keep 5.x.y html changes from the 5.x.y branch while resolving
      conflicts. If there are conflicts in the current version html files, regenerating the html files
      is the best way to solve it.
      If there are conflicts in the "*.rst" files, resolve them and generate HTML
      which will update the HTML for current version.

   d. Commit 5.x.y merge:

      After resolving the merge conflict:
      Commit the "*.rst" files and the generated 5.x.y HTML folders.

   e. Generate HTML for the current version:

    ```bash
    cd earthenterprise/docs/geedocs/docsrc
    rm -rf build
    make html
    ```

    Based on the current Open GEE version , it will generate the current version HTML
    documentation under _earthenterprise/docs/geedocs/_.

    Add those files to the commit:

    ```bash
    git add earthenterprise/docs/geedocs/5.x.z/*
    git commit
    ```

   f. Now you can push and create a pull request for these two commits.
      Note: keeping the "*.rst" files in a separate commit will make the PR review easier.

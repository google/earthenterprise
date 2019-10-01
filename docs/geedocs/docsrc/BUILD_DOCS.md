# Build Instructions for Earth Enterprise Documentation

OpenGEE Documentation from Release 5.3.2 is in rst(restructured text format).
Previously it was in html. To update Documentation, update the rst and generate
html using Sphinx.

## Prerequisites:

To build Documentation you need to install Sphinx:

```bash
sudo apt install python-pip
sudo pip install sphinx==1.7.5
sudo pip install sphinxprettysearchresults
```

## How documentation is Organized:

 * earthenterprise/docs/geedocs/docsrc contains index.rst, which includes all the
    index files for major topics.

 * docsrc/answer has the index files for all the topic folders.

 * Each major topic's docs are in a folder under docsrc/answer.


 For example, docsrc/index.rst includes fusionTutorial.rst (the index file of the
 fusionTutorial folder), which includes all the rst files in the fusionTutorial folder.

## Steps to Update Documentation:

 1) IF you are modifying content to an existing file:
     Make the changes in rst syntax and Generate html(step 3).

 2) IF you are adding a new file under an existing major topic, create an rst file
     under the topic folder and add that file in answer/topicName.rst where it fits in the
     topic order. Make the changes in rst syntax and generate html (step 3).

 3) Generate html:

    ```bash
    $ cd earthenterprise/docs/geedocs/docsrc
    $ rm -rf build
    $ make html
    ```

    Depending on your opengee version, appropriate html folders will be updated when
    you do the make command.

    For example, in 5.3.2 version, after make, you will see a lot of generated files
    under earthenterprise/docs/geedocs/5.3.2.

    Add the rst files that you added or changed under docsrc and the generated files
    under the release folder to your commit.

    ```bash
    $ git add earthenterprise/docs/geedocs/5.3.2/*
    $ git add earthenterprise/docs/geedocs/docsrc/*
    ```

    Commit your changes and create a PR.

__NOTE:__
## NOTE: When you merge your release branch in to your master with doc changes

Assume you have branched to release and your release branch is release_5.3.2
and you are making document changes that need to go in to the release_5.3.2
branch and master.

 1) Create a branch off release_5.3.2 branch:

    ```bash
    $ git checkout -b release_5.3.2 upstream/release_5.3.2
    $ git checkout -b changes_to_5.3.2
    ```

 2) Make your changes as in step 1 or step 2 above. Perform step 3 to generate html.
    This step will update earthenterprise/docs/geedocs/5.3.2

3) Commit rst and 5.3.2 html changes and create a PR to push to
   release_5.3.2 branch.

4) After your changes are merged in to release_5.3.2 branch,
   To merge doc changes to master:

   a) Update master in your fork:
      ```bash
      $ git checkout master
      $ git fetch upstream
      $ git pull upstream master ( to get the latest changes from upstream)
      $ git push origin master
      ```

   b) Create a branch off of master:
      ```bash
      $ git checkout master
      $ git checkout -b merge_5.3.2_to_master
      ```

   c) Pull release_5.3.2 changes:

      ```bash
      $ git pull upstream release_5.3.2
        From git://github.com/google/earthenterprise
        * branch            release_5.3.2 -> FETCH_HEAD
        Auto-merging docs/geedocs/docsrc/index.rst
        CONFLICT (content): Merge conflict in docs/geedocs/docsrc/index.rst
        Automatic merge failed; fix conflicts and then commit the result.
      ```

      There was a merge conflict in index.rst.

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
      update the documentation based on the current opengee release.

      Merges may become tricky if there are conflicts in the generated html.
      if for example, we are bring 5.3.2 changes in to master we will be having 5.3.2
      html changes. you can keep the change in 5.3.2 branch while resolving conflicts.
      Resolve conflicts in rst and generate html for master which will update 5.3.3
      html.


   d) Commit 5.3.2 merge:

      After fixing the merge conflict,
      ```bash
      $ git add index.rst (add the fixed conflict to the commit)
      $ git commit
      ```

      Now you have committed the rst changes made in 5.3.2 release and the generated
      5.3.2 html folders.

   e) Generate html for 5.3.3 (master):

    ```bash
    $ cd earthenterprise/docs/geedocs/docsrc
    $ rm -rf build
    $ make html
    ```

    Based on the current openGEE version (5.3.3), it will generate 5.3.3 html
    documentation under earthenterprise/docs/geedocs/5.3.3.

    Add those files to the commit
    ```bash
    $ git add earthenterprise/docs/geedocs/5.3.3/*
    $ git commit
    ```

   f) Now you can push and create a PR for these two commits

Merges may become tricky if there are conflicts in the generated html.
If for example, we are bring 5.3.2 changes in to master we will be having 5.3.2
html changes. you can keep the change in 5.3.2 branch while resolving conflicts.
Resolve conflicts in rst and generate html for master which will update 5.3.3
html.

# G'MIC-Qt: Contribute to filters translation

We describe here the steps you should follow
if you want to help with the Italian translation of the filters
(names, parameters, etc.).

## Step 1: Edit the `it.ts` file

* The `.ts` files are located in the
[translations/filters](https://github.com/c-koi/gmic-qt/tree/master/translations/filters)
folder.

* They contain only automatic translations for now.
Therefore they really need some editing!

* Editing can be done using a simple text editor, although the
`linguist-qt5` tool may help.
 
## Step 2: Produce an `it.qm` file
* `it.qm` file is the "binary" version of `it.ts`.
* In the `translations/filters` folder, run:

```shell
$ lrelease-qt5 -compress fr.ts
```

* Check that this indeed produced the file `it.qm`.

## Step 3: Test it!
* Add your `it.qm` file in the *work in progress* set of translations.
 For this, add a line in the file `wip_translations.qrc` in the root folder.

```xml
<RCC>
    <qresource prefix="/">
       <file>translations/filters/fr.qm</file>
       <file>translations/filters/it.qm</file>
    </qresource>
</RCC>
```
* At last, you need to enable WIP translations before
compiling the plugin.

```shell
$ qmake-qt5 TEST_FILTERS_QM=on
$ make
```

## Step 4: Submit a Pull Request
* Your PR should only include the `it.ts` file and the updated
 `wip_translations.qrc`.
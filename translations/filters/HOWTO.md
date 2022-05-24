# G'MIC-Qt: Contribute to filters translation

We describe here the steps you should follow
if you want to help with the *Italian* translation of the filters
(names, parameters, etc.).

## Step 1: Edit the `it.csv` file

* The `.csv` files are located in the
[translations/filters](https://github.com/c-koi/gmic-qt/tree/master/translations/filters)
folder.

* They contain only automatic translations for now (except for French and Chinese).
Therefore they really need some editing!

* Editing can be done using a simple text editor.

* The CSV file contains up to 3 columns :

```txt
Original text , Translation [, Filter name] 
```
 
Filter name may be used to disambiguate the translation by providing a context.

## Step 2: Enable the language in `translations/filters/Makefile`

* Edit the `Makefile` to add the `.qm` file to the list.

```txt
 QM = fr.qm zh.qm it.qm
```

* In the `translations/filters` folder, run make:

```shell
$ make
```

* Check that this indeed produced the file `it.qm`.

## Step 3: Test it!
* If not already there, add your `it.qm` file in the *work in progress* set of translations.
 For this, add a line in the file `wip_translations.qrc` in the root folder.

```xml
<RCC>
    <qresource prefix="/">
       <file>translations/filters/fr.qm</file>
       <file>translations/filters/zh.qm</file>
       <file>translations/filters/it.qm</file>
    </qresource>
</RCC>
```
* Caution : you need to check "Translate filters (WIP)" in the settings dialog.
As a WIP, it is disabled by default.

## Step 4: Submit a Pull Request

* Your PR should only include the `it.csv`, the `Makefile`, and the updated
 `wip_translations.qrc`.
# G'MIC-Qt standalone version HOWTO

The G'MIC-Qt GUI accepts several options on its command line, thus allowing simple batch processing.
In short, command line usage is as follows:

 * `gmic_qt [OPTIONS ...] [INPUT_FILES ...]`

More than basic single input file name specification, options allow batch processing using the G'MIC filters, with or without the help of the GUI.

## Command line parameters

### Option `-o --output FILE`

Instead of displaying the output image in a dialog with a "Save as..." button, the application will save the filter output in the specified file. If multiple input files are provided, it is
suggested to include one of the `%b` or `%f` placeholders in the specified filename so that all output images will be written to distinct files :

  - `%b` is the input file basename, that is the filename with no extension and no path.
  - `%f` is the input file filename (without path). 

 
#### Examples

```sh
# Launch the GUI, save output to output.png
$ ./gmic_qt --output output.png input.png
# Select a filter and its parameters twice (i.e. once for each input), save each output to a distinct file. 
$ ./gmic_qt --output processed-%f input1.png input2.png
```

### Option `-q --quality N`

Set the quality of JPEG output files to N (N in 0..100).

#### Example

```sh
# Select a filter through the GUI and save the result using 85 quality factor.
$ ./gmic_qt --quality 85 --output blured-gmicky.jpg gmicky.png
# Select a filter through the GUI, do not ask for JPEG quality when "Saving as..."
$ ./gmic_qt --quality 85  gmicky.png
```

### Option `-r --repeat`

Use last applied filter and parameters.

#### Example

```sh
# Select a filter & its parameters from GUI, then apply.
$ ./gmic_qt gmicky.png
# Call the GUI with previously applied filter & parameters, output will be written to result.png
$ ./gmic_qt --repeat --output result.png  hat.png
```

### Option `-p --path FILTER_PATH|FILTER_NAME`

  - Select filter from a full path in the filter tree or from its name (if unique).
   A filter path begins with `/`,  like for example `/Black & White/Charcoal`.

#### Example

```sh
# Launch GUI with selected filter
$ ./gmic_qt --path "/Black & White/Charcoal" gmicky.png
# Apply Charcoal filter with default parameters to image, then
# save result to charcoal-gmic.png
$ ./gmic_qt --path Charcoal --ouput charcoal-%f gmicky.png
```

### Option `-c --command "GMIC COMMAND"`

Run the gmic command on input image(s).

If a filter name/path is provided (option `-p`) and the command matches with this filter, or no filter name/path is provided and the command matches with some filter, then the parameters are completed using the filter's defaults.

If the command does not match with a filter, the option `--apply` (see below) is highly recommended as the GUI will not be of any help. Of course, batch processing is possible through the extra option `--output`.

#### Examples

```sh
# Launch GUI with filter "/Black & White/Charcoal", using 30 as its first parameter and default values otherwise.
$ ./gmic_qt -c "fx_charcoal 30" input.png
# Batch process to blur several images, showing each output with a "Save as..." option
$ ./gmic_qt -c "blur 10" images/*.png
# Batch process to blur several images, writing output images in distinct files
$ ./gmic_qt -o output/%b-blurred.jpg -c "blur 10" images/*.png
```

### Option `--apply`

Apply filter or command without showing the main plugin dialog for filter & parameters selection. One of the following options must therefore be present: `--path`, `--command` or `--repeat`.

#### Examples

```sh
# Shows up the resulting image dialog (image to be saved).
$ ./gmic_qt --apply -c "blur 10"  input.png
# Just do the processing, no question asked
$ ./gmic_qt --apply --path "Black & White/Charcoal" --output output.jpg input.jpg
# Select a filter and parameters through GUI, than batch process
$ ./gmic_qt --output test.png inputs/input1.png
$ ./gmic_qt --repeat --apply --ouput output/%f input/*.png
```

In fact, there is a command dedicated to the latter sample use case: `--first`

### Option `--reapply-first -R`

Launch the GUI once for the first input file, then reapply selected filter and parameters to all other files like `--repeat --apply` would do.

#### Examples

```sh
# Select a filter and parameters through GUI, than batch process to all input files
$ ./gmic_qt --reapply-first --output output/%f input/*.png
# Tune the Charcoal filter's parameters through the GUI, than batch process to all input files
$ ./gmic_qt -R -p "Charcoal" -o output/%f input/*.png
```

### Option `--show-last`

Print last applied plugin parameters

#### Example

```sh
$ ./gmic_qt --apply -p "Charcoal" -c "fx_charcoal 56" input.png
$ ./gmic_qt --show-last
Path: /Black & White/Charcoal
Name: Charcoal
Command: fx_charcoal 56,70,170,0,1,0,50,70,255,255,255,0,0,0,0,0,50,50
InputMode: 1
OutputMode: 0
```

### Option `--show-last-after`

Print last applied plugin parameters after filter execution. (Indeed, some filters may change the value of there parameters.)




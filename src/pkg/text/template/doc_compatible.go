/*

Text and spaces

By default, all text between actions is copied verbatim when the template is
executed. For example, the string " items are made of " in the example above appears
on standard output when the program is run.

However, to aid in formatting template source code, if an action's left delimiter
(by default "{{") is followed immediately by a minus sign and ASCII space character
("{{- "), all trailing white space is trimmed from the immediately preceding text.
Similarly, if the right delimiter ("}}") is preceded by a space and minus sign
(" -}}"), all leading white space is trimmed from the immediately following text.
In these trim markers, the ASCII space must be present; "{{-3}}" parses as an
action containing the number -3.

For instance, when executing the template whose source is

	"{{23 -}} < {{- 45}}"

the generated output would be

	"23<45"

For this trimming, the definition of white space characters is the same as in Go:
space, horizontal tab, carriage return, and newline.

*/

package template

<!DOCTYPE style-sheet PUBLIC "-//James Clark//DTD DSSSL Style Sheet//EN" [
  <!ENTITY %  output.print.pdf "IGNORE">
  <!ENTITY %  output.print.eps "IGNORE">
  <!ENTITY %  draft "INCLUDE">
  <!ENTITY % html "IGNORE">
  <![%html;[
    <!ENTITY % print "IGNORE">
    <!ENTITY docbook.dsl SYSTEM "html/docbook.dsl" CDATA dsssl>
  ]]>
  <!ENTITY % print "INCLUDE">
  <![%print;[
    <!ENTITY docbook.dsl SYSTEM "print/docbook.dsl" CDATA dsssl>
  ]]>
]>

<!-- Local document specific customizations. -->

<style-sheet>
<style-specification id="print" use="docbook">
<style-specification-body>

<![%output.print.pdf;[
(define %graphic-default-extension% "pdf") ;;; just change this to png
]]>

<![%output.print.eps;[
(define %graphic-default-extension% "eps")
]]>

(declare-characteristic page-two-side?
   "UNREGISTERED::OpenJade//Characteristic::page-two-side?" #f)

(define (book-titlepage-recto-elements)
  (list (normalize "title") 
	(normalize "subtitle") 
	(normalize "graphic") 
	(normalize "mediaobject")
	(normalize "corpauthor") 
	(normalize "authorgroup") 
	(normalize "author") 
	(normalize "editor")
	(normalize "copyright")
	(normalize "legalnotice")
	(normalize "printhistory")))



;; Abstracts should be italic
(element abstract
  (make display-group
    font-posture: 'italic
    font-size: 10pt
    space-before: %block-sep%
    space-after: %block-sep%
    start-indent: %body-start-indent%
    (process-children)))

;;; This is to circomvent a bug in some style sheets
;;; print/dblists.dsl
(element (varlistentry term)
    (make paragraph
	  space-before: (if (first-sibling?)
			    %block-sep%
			    0pt)
	  keep-with-next?: #t
	  first-line-start-indent: 0pt
	  start-indent: (inherited-start-indent) ; was 0pt
	  (process-children)))

;; put things where they are in the flow
(define (float-object node)
  ;; you could redefine this to make only figures float, or only tables,
  ;; or whatever...
  #f)

; (define (page-center-footer gi)
;   (make sequence
;     font-posture: 'italic
;     font-family-name: "Helvetica"
;     font-size: 8pt
;     (literal "IBM Confidential")))
; (define (first-page-center-header gi)
;   (make sequence
;     font-posture: 'italic
;     font-family-name: "Helvetica"
;     font-size: 8pt
;     (literal "IBM Confidential")))

;; These should be common to all output modes
(define %refentry-xref-italic%
  ;; Use italic text when cross-referencing RefEntrys?
  #f)
(define %refentry-xref-manvolnum%
  ;; Output manvolnum as part of RefEntry cross-reference?
  #f)
(define %section-autolabel%
  ;;Do you want enumerated sections? (E.g, 1.1, 1.1.1, 1.2, etc.)
  #f)
(define %refentry-new-page%
  ;; If true, each 'RefEntry' begins on a new page.
  #t)

(define %refentry-page-number-restart%
  ;; If true, each 'RefEntry' begins on a new page.
  #f)

;; override refentry so that we can change page number resets
(element refentry 
  (make display-group
    keep: %refentry-keep%
    (if (or %refentry-new-page%
	    (node-list=? (current-node) (sgml-root-element)))
	(make simple-page-sequence
	  page-number-restart?: %refentry-page-number-restart%
	  page-n-columns: %page-n-columns%
	  page-number-format: ($page-number-format$)
	  use: default-text-style
	  left-header:   ($left-header$)
	  center-header: ($center-header$)
	  right-header:  ($right-header$)
	  left-footer:   ($left-footer$)
	  center-footer: ($center-footer$)
	  right-footer:  ($right-footer$)
	  input-whitespace-treatment: 'collapse
	  quadding: %default-quadding%
	  ($refentry-title$)
	  (process-children))
	(make sequence
	  ($refentry-title$)
	  ($block-container$)))
    (make-endnotes)))

(element application ($mono-seq$))
(element envar ($mono-seq$))
(element interface ($mono-seq$))
(element interfacedefinition ($mono-seq$))
;; specific

(define default-backend
  ;; This parameter sets the default backend.
  tex)
(define %visual-acuity%
  ;; General measure of document text size
  ;; "tiny", "normal", "presbyopic", and "large-type"
  "normal")
(define %two-side%
  ;; Is two-sided output being produced?
  #t)
(define %titlepage-n-columns%
  ;; Sets the number of columns on the title page
  1)
(define %page-n-columns%
  ;; Sets the number of columns on each page
  1)
(define %page-balance-columns?%
  ;; Balance columns on pages?
  #t)
(define %default-quadding%  
  ;; The default quadding
  'start)

(define %left-margin%
  ;; Width of left margin
  4pi)
(define %right-margin%
  ;; Width of the right margin
  8pi)
(define %top-margin%
  ;;How big do you want the margin at the top?
  12pi)
(define %bottom-margin%
  ;; How big do you want the margin at the bottom?
  2pi)
(define %header-margin%
  ;; height of the header margin
  8pi)
(define %body-start-indent%
  ;; Default indent of body text
  2pi)
(define %show-ulinks%
  ;; Display URLs after ULinks?
  #f)
;; This does not seem to work
(define %footnote-ulinks%
  ;; Generate footnotes for ULinks?
  #f)
(define bop-footnotes
  ;; Make "bottom-of-page" footnotes?
  #t)
(define %footnote-size-factor%
  ;; Footnote font scaling factor
  0.9)
(define %show-comments%
  ;; Display Comment elements?
  #t)
(define %hyphenation%
  ;; Allow automatic hyphenation?
  #t)
(define ($object-titles-after$)
  ;; List of objects who's titles go after the object
  ;;'())
  (list (normalize "figure")))
(define %title-font-family%
  ;; What font would you like for titles?
  "Helvetica")
(define %body-font-family%
  ;; What font would you like for the body?
  "Palatino")
(define %mono-font-family%
  ;; What font would you like for mono-seq?
  "Courier New")
(define %hsize-bump-factor%
  1.1)
(define %block-sep%
  ;; Distance between block-elements
  (* %para-sep% 0.5))
(define %head-before-factor%
  ;;  Factor used to calculate space above a title
  0.01)
(define %head-after-factor%
  ;; Factor used to calculate space below a title
  0.01)

(define formal-object-float
  ;; If '#t', formal objects will float if floating is supported by the
  ;; backend. At present, only the TeX backend supports floats.
  #t)

(element phrase
  (let ((role (if (attribute-string (normalize "role"))
		  (attribute-string (normalize "role"))
		   (normalize "default"))))
    (cond
     ((equal? (normalize role) (normalize "del")) ($score-seq$ 'through))
     ((equal? (normalize role) (normalize "add")) ($score-seq$ 'after))
     (else ($charseq$)))))

;;
;; Biblio Rules
;;
(define biblio-filter-used #f)

;; this is borken if there are no XREFS
;;(define biblio-citation-check #t)

</style-specification-body>
</style-specification>

<style-specification id="html" use="docbook">
<style-specification-body>

;; These should be common to all output modes
(define %refentry-xref-italic%
  ;; Use italic text when cross-referencing RefEntrys?
  #t)
(define %refentry-xref-manvolnum%
  ;; Output manvolnum as part of RefEntry cross-reference?
  #f)
(define %section-autolabel%
  ;;Do you want enumerated sections? (E.g, 1.1, 1.1.1, 1.2, etc.)
  #t)

;; specific

;; Change depth of article TOC to include my section level 2
(define (toc-depth nd)
  (if (string=? (gi nd) (normalize "article"))
      4
      7))

(define %graphic-default-extension%
  ;;What is the default extension for images?
  "png")
(define %generate-article-toc%
  ;; Should a Table of Contents be produced for Articles?
  #t)
(define %header-navigation%
  ;; Should navigation links be added to the top of each page?
  #t)
(define %footer-navigation%
  ;; Should navigation links be added to the bottom of each page?
  #t)
(define %shade-verbatim% 
  ;; Should verbatim environments be shaded?
  #f)
(define nochunks
  ;; Suppress chunking of output pages
  #t)
(define rootchunk
  ;; Make a chunk for the root element when nochunks is used
  #f)
(define %body-attr%
  ;; What attributes should be hung off of BODY?
  (list
   (list "BGCOLOR" "#FFFFFF")
   (list "TEXT" "#000000")
   (list "LINK" "#0000FF")
   (list "VLINK" "#840084")
   (list "ALINK" "#0000FF")))
(define %html-ext%
  ;; Default extension for HTML output files
  ".html")

(define %html-pubid%
  ;; What public ID are you declaring your HTML compliant with?
  "-//W3C//DTD HTML 4.01 Transitional//EN")

(define %html-prefix%
  ;; Add the specified prefix to HTML output filenames
  "")
(define %root-filename%
  ;; Name for the root HTML document
  #f)
(define %show-comments%
  ;; Display Comment elements?
  #t)

;;
;; Biblio Rules
;;
(define biblio-filter-used #t)
;; this is borken if there are no XREFS
;;(define biblio-citation-check #t)


</style-specification-body>
</style-specification>
<external-specification id="docbook" document="docbook.dsl">
</style-sheet>

<!--
Local Variables:
mode: scheme
End:
-->

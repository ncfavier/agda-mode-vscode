open Belt;
module Impl = (Editor: Sig.Editor) => {
  module Decoration = Decoration.Impl(Editor);
  type t = {
    index: int,
    mutable range: Editor.Range.t,
    decorations: array(Editor.Decoration.t),
  };

  // NOTE: helper function of `makeMany`, returns a thunk
  let make =
      (editor: Editor.editor, diff: SourceFile.Diff.t)
      : (unit => Promise.t(t)) => {
    // modify the text buffer base on the Diff
    () => {
      let originalRange =
        Editor.Range.make(
          Editor.pointAtOffset(editor, fst(diff.originalRange)),
          Editor.pointAtOffset(editor, snd(diff.originalRange)),
        );
      Editor.setText(editor, originalRange, diff.content)
      ->Promise.map(_ => {
          let modifiedRange =
            Editor.Range.make(
              Editor.pointAtOffset(editor, fst(diff.modifiedRange)),
              Editor.pointAtOffset(editor, snd(diff.modifiedRange)),
            );
          let decorations =
            Decoration.decorateHole(editor, modifiedRange, diff.index);
          {index: diff.index, range: modifiedRange, decorations};
        });
    };
  };

  // make an array of Goal.t with given goal indices
  // modifies the text buffer along the way
  let makeMany =
      (editor: Editor.editor, indices: array(int)): Promise.t(array(t)) => {
    let filePath =
      Editor.getFileName(editor)->Option.getWithDefault("unnamed.agda");
    let source = Editor.getText(editor);
    let diffs = SourceFile.parse(indices, filePath, source);
    // scan through the diffs to modify the text buffer one by one

    diffs->Array.map(make(editor))->Util.oneByOne;
  };

  let getInnerRange = (self, editor) =>
    Editor.Range.make(
      Editor.Point.translate(Editor.Range.start(self.range), 0, 2),
      Editor.pointAtOffset(
        editor,
        Editor.offsetAtPoint(editor, Editor.Range.end_(self.range)) - 2,
      ),
    );

  let getContent = (self, editor) => {
    let innerRange = getInnerRange(self, editor);
    Editor.getTextInRange(editor, innerRange)->Parser.userInput;
  };

  let setContent = (self, editor, text) => {
    let innerRange = getInnerRange(self, editor);

    // let paddingSpaces =
    //   Js.String.repeat(String.length(string_of_int(self.index)), " ");

    Editor.setText(editor, innerRange, " " ++ text ++ " ");
  };

  let buildHaskellRange = (editor, self, old, filepath: string) => {
    let start = Editor.Range.start(self.range);
    let startIndex = Editor.offsetAtPoint(editor, start);

    let end_ = Editor.Range.end_(self.range);
    let endIndex = Editor.offsetAtPoint(editor, end_);

    let startIndex' = string_of_int(startIndex + 3);
    let startRow = string_of_int(Editor.Point.line(start) + 1);
    let startColumn = string_of_int(Editor.Point.column(start) + 3);
    let startPart = {j|$(startIndex') $(startRow) $(startColumn)|j};
    let endIndex' = string_of_int(endIndex - 3);
    let endRow = string_of_int(Editor.Point.line(end_) + 1);
    let endColumn = string_of_int(Editor.Point.column(end_) - 1);
    let endPart = {j|$(endIndex') $(endRow) $(endColumn)|j};

    if (old) {
      {j|(Range [Interval (Pn (Just (mkAbsolute "$(filepath)")) $(startPart)) (Pn (Just (mkAbsolute "$(filepath)")) $(endPart))])|j}
      // before (not including) 2.5.1
    } else {
      {j|(intervalsToRange (Just (mkAbsolute "$(filepath)")) [Interval (Pn () $(startPart)) (Pn () $(endPart))])|j}
      // after 2.5.1
    };
  };

  let destroy = self => {
    self.decorations->Array.forEach(Editor.Decoration.destroy);
  };
};
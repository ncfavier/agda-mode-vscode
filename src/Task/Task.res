type rec t =
  | DispatchCommand(Command.t)
  // Agda
  | AgdaRequest(Request.t)
  // View
  | ViewRequest(View.Request.t, View.Response.t => list<t>)
  // Misc
  | Decoration(Decoration.action)
  // | Goal(Goal.action<t>)
  | WithState(State.t => unit)
  | WithStateP(State.t => Promise.t<list<t>>)
  | Destroy
  | Debug(string)

let toString = x =>
  switch x {
  | DispatchCommand(cmd) => "Command[" ++ (Command.toString(cmd) ++ "]")
  | Destroy => "Destroy"
  | AgdaRequest(_req) => "AgdaRequest"
  | ViewRequest(_, _) => "ViewRequest"
  // | Goal(Instantiate(_)) => "Goal[Instantiate]"
  // | Goal(UpdateRange) => "Goal[UpdateRange]"
  // | Goal(Next) => "Goal[Next]"
  // | Goal(Previous) => "Goal[Previous]"
  // | Goal(Modify(_, _)) => "Goal[Modify]"
  // | Goal(SaveCursor) => "Goal[SaveCursor]"
  // | Goal(RestoreCursor) => "Goal[RestoreCursor]"
  // | Goal(SetCursor(_)) => "Goal[SetCursor]"
  // | Goal(JumpToOffset(_)) => "Goal[JumpToOffset]"
  // | Goal(RemoveBoundaryAndDestroy(_)) => "Goal[RemoveBoundaryAndDestroy]"
  // | Goal(ReplaceWithLines(_, _)) => "Goal[ReplaceWithLines]"
  // | Goal(ReplaceWithLambda(_, _)) => "Goal[ReplaceWithLambda]"
  // | Goal(LocalOrGlobal2(_, _, _)) => "Goal[LocalOrGlobal2]"
  // | Goal(LocalOrGlobal(_, _)) => "Goal[LocalOrGlobal]"
  | Decoration(AddViaPipe(_)) => "Decoration[AddViaPipe]"
  | Decoration(AddViaFile(_)) => "Decoration[AddViaFile]"
  | Decoration(Clear) => "Decoration[Clear]"
  | Decoration(Apply) => "Decoration[Apply]"
  | Decoration(ApplyExperimental) => "Decoration[ApplyExperimental]"
  | Decoration(Refresh) => "Decoration[Refresh]"
  | WithState(_) => "WithState"
  | WithStateP(_) => "WithStateP"
  | Debug(msg) => "Debug[" ++ (msg ++ "]")
  }

// Smart constructors for controlling the view

let viewEvent = event => WithStateP(
  state => state->State.sendEventToView(event)->Promise.map(_ => list{}),
)

// Header + Body
let display = (header, body) => viewEvent(Display(header, body))
let displayEmacs = (kind, header, body) =>
  viewEvent(Display(header, Emacs(kind, View.Header.toString(header), body)))

let displayOutOfGoalError = display(
  Error("Out of goal"),
  Plain("Please place the cursor in a goal"),
)

// Header + Prompt
let prompt = (header, prompt, callbackOnPromptSuccess: string => list<t>) => list{
  WithState(
    state => {
      // focus on the panel before prompting
      VSCode.Commands.setContext("agdaModePrompting", true)->ignore
      state.view->View__Controller.focus
    },
  ),
  ViewRequest(
    Prompt(header, prompt),
    response => {
      let tasks = switch response {
      | PromptSuccess(result) => callbackOnPromptSuccess(result)
      | PromptInterrupted => list{}
      }
      Belt.List.concat(
        tasks,
        list{
          WithState(
            state => {
              // put the focus back to the editor after prompting
              VSCode.Commands.setContext("agdaModePrompting", false)->ignore
              state.editor->VSCode.TextEditor.document->Editor.focus
            },
          ),
        },
      )
    },
  ),
}

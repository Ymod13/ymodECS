const engineMessage: string = "ymodECS TypeScript active!";
console.log(engineMessage);

const game_messages = document.getElementById("game_messages")!;
document.getElementById("start")!.addEventListener("click", (ev) => {
    game_messages.setInnerRML("Button clicked!");
    console.log("click a", ev.params["mouse_x"], ev.params["mouse_y"]);
});


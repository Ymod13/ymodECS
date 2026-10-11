"use strict";
const engineMessage = "ymodECS TypeScript attivo!";
console.log(engineMessage);
const game_status = document.getElementById("game_status");
document.getElementById("start").addEventListener("click", (ev) => {
    game_status.setInnerRML("avvio in corso...");
    console.log("click a", ev.params["mouse_x"], ev.params["mouse_y"]);
});
//# sourceMappingURL=Test.js.map
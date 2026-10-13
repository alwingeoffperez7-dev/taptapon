// Small helpers shared by student.js and admin.js.

// Same as the service: the daily cap resets at Manila midnight.
export const manilaDay = (ms) => new Intl.DateTimeFormat("en-CA", { timeZone: "Asia/Manila" }).format(ms);
export const when = (ms) => new Date(ms).toLocaleString("en-PH", {
  timeZone: "Asia/Manila", month: "short", day: "numeric", hour: "numeric", minute: "2-digit",
});

// Builds an element with textContent (never innerHTML), so data from the database can't inject HTML.
export function el(tag, className, text) {
  const node = document.createElement(tag);
  if (className) node.className = className;
  if (text !== undefined) node.textContent = text;
  return node;
}
export function row(title, sub, right, rightClass = "") {
  const li = el("li");
  const main = el("div", "main");
  main.append(el("div", "title", title));
  if (sub) main.append(el("div", "sub", sub));
  li.append(main);
  if (right instanceof Node) li.append(right);
  else if (right !== undefined) li.append(el("div", `right ${rightClass}`, right));
  return li;
}
export const emptyRow = (text) => el("li", "empty", text);

// Firebase returns query results in ascending order; snap.forEach keeps that order.
export function toList(snap) {
  const list = [];
  snap.forEach((child) => { list.push({ id: child.key, ...child.val() }); });
  return list;
}

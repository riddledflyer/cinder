import { useEffect, useMemo, useState, type ReactNode } from "react";
import * as AlertDialog from "@radix-ui/react-alert-dialog";
import { RotateCcw } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Checkbox } from "@/components/ui/checkbox";
import {
  SEED,
  expandSelection,
  isDeletable,
  kindLabel,
  loadRemoved,
  lockReason,
  lowId,
  saveRemoved,
  shelfOf,
  type Kind,
  type Shelf,
  type TitleEntry,
} from "@/lib/library";
import { cn } from "@/lib/cn";

const VERSION = "3.50";
const COLS = 4;

type Mode = "titles" | "install";

type CiaEntry = {
  id: string;
  name: string;
  publisher: string;
  bytes: number;
  kind: Kind;
  iconId: string;
  version: string;
  productCode: string;
};

const CIAS: CiaEntry[] = [
  { id: "cia-moss", name: "Moss Circuit", publisher: "Lowland", bytes: 2140 * 1024 * 1024, kind: "game", iconId: "0004000000A13000", version: "1.0.0", productCode: "CTR-P-MOSS" },
  { id: "cia-salt", name: "Salt Map", publisher: "Brine", bytes: 1904 * 1024 * 1024, kind: "game", iconId: "0004000000A17000", version: "1.0.0", productCode: "CTR-P-SALT" },
  { id: "cia-harbor", name: "Harbor Line", publisher: "Tideglass", bytes: 1640 * 1024 * 1024, kind: "game", iconId: "0004000000A11000", version: "1.0.0", productCode: "CTR-P-HARB" },
  { id: "cia-ferry", name: "Night Ferry", publisher: "Lantern Row", bytes: 1120 * 1024 * 1024, kind: "game", iconId: "0004000000A15000", version: "1.2.0", productCode: "CTR-P-FERR" },
  { id: "cia-kite", name: "Paper Kite", publisher: "Northwind", bytes: 820 * 1024 * 1024, kind: "game", iconId: "0004000000A12000", version: "1.1.0", productCode: "CTR-P-KITE" },
  { id: "cia-dlc", name: "Moss Circuit", publisher: "Lowland", bytes: 208 * 1024 * 1024, kind: "dlc", iconId: "0004000000A13000", version: "1.0.0", productCode: "CTR-M-MOSS" },
  { id: "cia-loom", name: "Pocket Loom", publisher: "Heddle", bytes: 18 * 1024 * 1024, kind: "dsiware", iconId: "000480044B4C4D00", version: "1.0.0", productCode: "TWL-P-LOOM" },
];

function charsOf(text: string) {
  return Array.from(text);
}

function clipChars(text: string, limit: number) {
  const chars = charsOf(text);
  if (chars.length <= limit) return text;
  return `${chars.slice(0, limit).join("")}...`;
}

function wrapName(text: string, limit: number) {
  const chars = charsOf(text);
  if (chars.length <= limit) return text;
  const lines: string[] = [];
  let line = "";
  let col = 0;
  for (const ch of chars) {
    if (ch === " " && col === 0) continue;
    if (col >= limit) {
      const space = line.lastIndexOf(" ");
      if (space > 0) {
        lines.push(line.slice(0, space));
        line = line.slice(space + 1) + ch;
        col = charsOf(line).length;
      } else {
        lines.push(line);
        line = ch === " " ? "" : ch;
        col = charsOf(line).length;
      }
      continue;
    }
    line += ch;
    col += 1;
  }
  if (line) lines.push(line);
  return lines.join("\n");
}

function consoleBytes(bytes: number) {
  if (bytes < 1024) return `${bytes}B`;
  if (bytes < 1024 * 1024) return `${Math.ceil(bytes / 1024)}KB`;
  if (bytes < 1024 * 1024 * 1024) {
    const tenths = Math.floor((bytes * 10) / (1024 * 1024));
    return `${Math.floor(tenths / 10)}.${tenths % 10}MB`;
  }
  const tenths = Math.floor((bytes * 10) / (1024 * 1024 * 1024));
  return `${Math.floor(tenths / 10)}.${tenths % 10}GB`;
}

function bySize<T extends { bytes: number; name: string }>(items: T[]) {
  return [...items].sort((a, b) => b.bytes - a.bytes || a.name.localeCompare(b.name));
}

function isLicensed(publisher: string) {
  const name = publisher.trim().toLowerCase();
  return name !== "" && name !== "-" && name !== "homebrew" && name !== "devkitpro";
}

function shelfRank(title: TitleEntry) {
  if (!isLicensed(title.publisher)) return 3;
  if (title.kind === "update") return 1;
  if (title.kind === "dlc") return 2;
  return 0;
}

function groupBlocks(items: TitleEntry[], shelf: Shelf) {
  if (shelf === "nds") return [{ label: "", items }];
  const labels = ["Games", "Updates", "DLC", "Homebrew"];
  return [0, 1, 2, 3]
    .map((rank) => ({ label: labels[rank], items: items.filter((title) => shelfRank(title) === rank) }))
    .filter((group) => group.items.length > 0);
}

function shelfOrder(items: TitleEntry[], shelf: Shelf) {
  const sized = bySize(items);
  if (shelf === "nds") return sized;
  return [...sized].sort((a, b) => shelfRank(a) - shelfRank(b) || b.bytes - a.bytes || a.name.localeCompare(b.name));
}

function parentOf(title: TitleEntry, installed: TitleEntry[]) {
  if (title.kind !== "dlc") return null;
  const low = lowId(title.id);
  return (
    installed.find((item) => (item.kind === "game" || item.kind === "demo") && lowId(item.id) === low) ?? null
  );
}

function clockText(now: Date) {
  const months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
  let hour = now.getHours();
  const am = hour < 12;
  hour = hour % 12;
  if (hour === 0) hour = 12;
  const minute = String(now.getMinutes()).padStart(2, "0");
  return `${months[now.getMonth()]} ${now.getDate()} ${now.getFullYear()}  ${hour}:${minute} ${am ? "AM" : "PM"}`;
}

export function CinderApp() {
  const [removed, setRemoved] = useState<string[]>([]);
  const [shelf, setShelf] = useState<Shelf>("3ds");
  const [selected, setSelected] = useState<string[]>([]);
  const [cursorId, setCursorId] = useState<string | null>(null);
  const [includeRelated, setIncludeRelated] = useState(true);
  const [freed, setFreed] = useState(0);
  const [dialog, setDialog] = useState<"closed" | "confirm" | "working" | "done">("closed");
  const [pendingIds, setPendingIds] = useState<string[]>([]);
  const [grid, setGrid] = useState(false);
  const [mode, setMode] = useState<Mode>("titles");
  const [ciaSelected, setCiaSelected] = useState<string[]>([]);
  const [ciaCursor, setCiaCursor] = useState(0);
  const [now, setNow] = useState(() => new Date());

  useEffect(() => {
    setRemoved(loadRemoved());
  }, []);

  useEffect(() => {
    const id = window.setInterval(() => setNow(new Date()), 1000);
    return () => window.clearInterval(id);
  }, []);

  const installed = useMemo(
    () => SEED.filter((title) => !removed.includes(title.id) && !pendingIds.includes(title.id)),
    [removed, pendingIds],
  );

  const visible = useMemo(
    () => shelfOrder(installed.filter((title) => shelfOf(title) === shelf), shelf),
    [installed, shelf],
  );

  const cursor = visible.find((title) => title.id === cursorId) ?? visible[0] ?? null;
  const selectedTitles = installed.filter((title) => selected.includes(title.id));
  const selectedBytes = selectedTitles.reduce((sum, title) => sum + title.bytes, 0);
  const planned = expandSelection(selectedTitles, installed, includeRelated);
  const plannedBytes = planned.reduce((sum, title) => sum + title.bytes, 0);
  const cias = useMemo(() => bySize(CIAS), []);
  const cia = cias[Math.min(ciaCursor, Math.max(0, cias.length - 1))] ?? null;
  const ciaBytes = cias.filter((item) => ciaSelected.includes(item.id)).reduce((sum, item) => sum + item.bytes, 0);
  const counts = {
    "3ds": installed.filter((title) => shelfOf(title) === "3ds").length,
    nds: installed.filter((title) => shelfOf(title) === "nds").length,
  };

  useEffect(() => {
    function move(list: { id: string }[], current: number, dx: number, dy: number) {
      if (list.length === 0) return 0;
      let next = current;
      if (!grid) {
        if (dy < 0) next = Math.max(0, current - 1);
        if (dy > 0) next = Math.min(list.length - 1, current + 1);
      } else {
        if (dx < 0) next = Math.max(0, current - 1);
        if (dx > 0) next = Math.min(list.length - 1, current + 1);
        if (dy < 0 && current >= COLS) next = current - COLS;
        if (dy > 0 && current + COLS < list.length) next = current + COLS;
      }
      return next;
    }

    function onKey(event: KeyboardEvent) {
      const target = event.target;
      if (target instanceof HTMLElement && (target.tagName === "INPUT" || target.tagName === "TEXTAREA")) return;
      if (dialog === "working") return;
      const key = event.key.toLowerCase();
      const dx = key === "arrowleft" ? -1 : key === "arrowright" ? 1 : 0;
      const dy = key === "arrowup" ? -1 : key === "arrowdown" ? 1 : 0;
      if (dx || dy) {
        event.preventDefault();
        if (mode === "install") {
          const index = move(cias, ciaCursor, dx, dy);
          setCiaCursor(index);
          document.getElementById(`cia-${cias[index]?.id}`)?.scrollIntoView({ block: "nearest" });
        } else {
          if (visible.length === 0) return;
          const current = Math.max(0, visible.findIndex((title) => title.id === cursor?.id));
          const index = move(visible, current, dx, dy);
          const id = visible[index]?.id;
          if (!id) return;
          setCursorId(id);
          document.getElementById(`title-${id}`)?.scrollIntoView({ block: "nearest" });
        }
      } else if (key === "a") {
        event.preventDefault();
        if (mode === "install" && cia) {
          setCiaSelected((current) =>
            current.includes(cia.id) ? current.filter((id) => id !== cia.id) : [...current, cia.id],
          );
        } else if (cursor && isDeletable(cursor)) toggle(cursor.id);
      } else if (key === "x") {
        event.preventDefault();
        if (mode === "install") {
          const ids = cias.map((item) => item.id);
          const allOn = ids.every((id) => ciaSelected.includes(id));
          setCiaSelected(allOn ? [] : ids);
        } else {
          const ids = visible.filter(isDeletable).map((title) => title.id);
          const allOn = ids.length > 0 && ids.every((id) => selected.includes(id));
          setSelected((current) =>
            allOn ? current.filter((id) => !ids.includes(id)) : [...new Set([...current, ...ids])],
          );
        }
      } else if (key === "l" && mode === "titles") {
        setShelf("3ds");
        setCursorId(null);
      } else if (key === "r" && mode === "titles") {
        setShelf("nds");
        setCursorId(null);
      } else if (key === "enter" && mode === "titles" && planned.length > 0 && dialog === "closed") {
        event.preventDefault();
        setDialog("confirm");
      } else if (key === "y") {
        setGrid((value) => !value);
      } else if (key === "i") {
        setMode((current) => (current === "install" ? "titles" : "install"));
      }
    }
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  });

  function toggle(id: string) {
    const title = installed.find((item) => item.id === id);
    if (!title || !isDeletable(title)) return;
    setSelected((current) => (current.includes(id) ? current.filter((item) => item !== id) : [...current, id]));
  }

  function switchShelf(next: Shelf) {
    setShelf(next);
    setCursorId(null);
    setMode("titles");
  }

  function runDelete() {
    const queue = expandSelection(selectedTitles, installed, includeRelated);
    if (queue.length === 0) return;
    setDialog("working");
    const reduce = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
    let index = 0;
    const step = () => {
      if (index >= queue.length) {
        const ids = queue.map((title) => title.id);
        const bytes = queue.reduce((sum, title) => sum + title.bytes, 0);
        setRemoved((current) => {
          const next = [...new Set([...current, ...ids])];
          saveRemoved(next);
          return next;
        });
        setSelected((current) => current.filter((id) => !ids.includes(id)));
        setFreed((current) => current + bytes);
        setPendingIds([]);
        setDialog("done");
        return;
      }
      setPendingIds(queue.slice(0, index + 1).map((title) => title.id));
      index += 1;
      window.setTimeout(step, reduce ? 0 : 90);
    };
    window.setTimeout(step, reduce ? 0 : 160);
  }

  function restoreLibrary() {
    setRemoved([]);
    saveRemoved([]);
    setSelected([]);
    setFreed(0);
    setPendingIds([]);
    setDialog("closed");
  }

  const shown = cursor ? parentOf(cursor, installed) : null;
  const detailName = shown?.name ?? cursor?.name ?? "";
  const detailPublisher = cursor?.publisher || shown?.publisher || "-";
  const detailIcon = shown?.id ?? cursor?.id ?? "";
  const locked = cursor ? lockReason(cursor) : null;

  return (
    <main className="min-h-dvh overflow-x-hidden bg-bg text-fg">
      <header className="mx-auto flex max-w-3xl items-end justify-between gap-4 px-4 pt-6 pb-4">
        <div className="flex items-center gap-3">
          <img src="/cinder-icon.png" alt="" width={56} height={56} className="size-14 rounded-xl" />
          <div>
            <p className="text-xs font-medium text-muted">Nintendo 3DS</p>
            <h1 className="text-2xl font-medium tracking-tight">Cinder</h1>
            <p className="text-xs text-muted">Version {VERSION}</p>
          </div>
        </div>
        <div className="flex shrink-0 gap-2">
          <a
            href={`/cinder-${VERSION}.cia`}
            download={`cinder-${VERSION}.cia`}
            className="press rounded-md bg-accent px-3 py-2 text-sm text-accent-fg"
          >
            cinder-{VERSION}.cia
          </a>
          <a
            href={`/cinder-${VERSION}-source.zip`}
            download={`cinder-${VERSION}-source.zip`}
            className="press rounded-md border border-border px-3 py-2 text-sm text-fg"
          >
            Source
          </a>
        </div>
      </header>

      <div className="mx-auto grid max-w-3xl gap-3 px-4 pb-28">
        <Screen wide>
          <div className="flex items-center justify-between gap-3 text-xs text-fg">
            <span>Cinder {VERSION}</span>
            <span className="flex items-center gap-2 text-muted">
              {clockText(now)}
              <Battery />
            </span>
          </div>
          <div className="my-2 h-px bg-border" />
          {mode === "install" ? (
            cia ? (
              <div className="flex min-h-0 flex-1 gap-3">
                <GameIcon id={cia.iconId} large />
                <div className="min-w-0 flex-1">
                  <p className="whitespace-pre-line text-sm">{wrapName(cia.name, 10)}</p>
                  <p className="mt-1 whitespace-pre-line text-xs text-muted">{wrapName(cia.publisher, 8)}</p>
                  <p className="mt-2 text-xs">
                    {kindLabel(cia.kind)} · v{cia.version} · {isLicensed(cia.publisher) ? "Licensed" : "Homebrew"}
                  </p>
                  <p className="text-xs">File {consoleBytes(cia.bytes)}</p>
                  <p className="font-mono text-[11px] text-muted">{cia.productCode}</p>
                </div>
                <p className="shrink-0 text-right text-[11px] text-muted">
                  SD 12.4GB / 29.8GB
                  <br />
                  NAND 180.2MB / 1.0GB
                </p>
              </div>
            ) : (
              <div className="flex-1 text-sm">
                <p>No CIAs found.</p>
                <p className="mt-1 text-xs text-muted">place cias in sdmc:/cias/ or sdmc:/</p>
              </div>
            )
          ) : cursor ? (
            <div className="flex min-h-0 flex-1 gap-3">
              <GameIcon id={detailIcon} large />
              <div className="min-w-0 flex-1">
                <p className="whitespace-pre-line text-sm">{wrapName(detailName, 10)}</p>
                <p className="mt-1 whitespace-pre-line text-xs text-muted">{wrapName(detailPublisher, 8)}</p>
                <p className="mt-2 text-xs">
                  {kindLabel(cursor.kind)} · v{cursor.version} · {isLicensed(cursor.publisher) ? "Licensed" : "Homebrew"}
                </p>
                <p className="text-xs">
                  {consoleBytes(cursor.bytes)} · {cursor.media === "sd" ? "SD card" : "NAND"}
                </p>
                <p className="font-mono text-[11px] text-muted">{cursor.productCode}</p>
                <p className="font-mono text-[11px] text-muted">{cursor.id}</p>
                {locked ? <p className="mt-1 text-[11px] text-muted">{locked}</p> : null}
              </div>
              <p className="shrink-0 text-right text-[11px] text-muted">
                SD 12.4GB / 29.8GB
                <br />
                NAND 180.2MB / 1.0GB
              </p>
            </div>
          ) : (
            <p className="flex-1 text-sm">{shelf === "nds" ? "No NDS/DSi software." : "No 3DS software."}</p>
          )}
          <Prompts
            items={
              mode === "install"
                ? [
                    ["A", "select"],
                    ["Y", grid ? "list" : "grid"],
                    ["X", "all"],
                    ["SELECT", "titles"],
                    ["START", "install"],
                    ["START+SELECT", "exit"],
                  ]
                : [
                    ["A", "select"],
                    ["Y", grid ? "list" : "grid"],
                    ["X", "all"],
                    ["SELECT", "install"],
                    ["START", "delete"],
                    ["START+SELECT", "exit"],
                  ]
            }
          />
        </Screen>

        <Screen>
          {mode === "titles" ? (
            <div className="mb-2 flex items-center gap-2">
              <div className="flex rounded-sm border border-border p-0.5" role="tablist" aria-label="Library">
                <ShelfButton current={shelf} value="3ds" count={counts["3ds"]} onSelect={switchShelf} label="3DS" />
                <ShelfButton current={shelf} value="nds" count={counts.nds} onSelect={switchShelf} label="NDS/DSi" />
              </div>
              <span className="text-xs text-muted">{visible.length}</span>
              <span className="ml-auto pr-7 text-xs tabular-nums">
                {selectedTitles.length} selected {consoleBytes(selectedBytes)}
              </span>
            </div>
          ) : (
            <div className="mb-2 flex items-center text-xs">
              <span>CIAs</span>
              <span className="ml-2 text-muted">{cias.length}</span>
              <span className="ml-auto pr-10 tabular-nums">
                {ciaSelected.length} selected {consoleBytes(ciaBytes)}
              </span>
            </div>
          )}
          <button
            type="button"
            aria-label={grid ? "List view" : "Grid view"}
            onClick={() => setGrid((value) => !value)}
            className="absolute top-2 right-2 grid size-5 place-items-center text-muted"
          >
            {grid ? <ListMark /> : <GridMark />}
          </button>
          {mode === "titles" ? (
            grid ? (
              <div className="min-h-0 flex-1 overflow-auto">
                {visible.length === 0 ? (
                  <p className="py-6 text-sm text-muted">
                    {shelf === "nds" ? "No NDS/DSi software." : "No 3DS software."}
                  </p>
                ) : (
                  groupBlocks(visible, shelf).map((group) => (
                    <div key={group.label || "nds"}>
                      {group.label ? <p className="px-1 py-1 text-xs text-muted">{group.label}</p> : null}
                      <div className="grid grid-cols-4 content-start gap-1">
                        {group.items.map((title) => {
                          const parent = parentOf(title, installed);
                          const on = selected.includes(title.id);
                          const focused = cursor?.id === title.id;
                          return (
                            <button
                              key={title.id}
                              id={`title-${title.id}`}
                              type="button"
                              aria-label={parent?.name ?? title.name}
                              onClick={() => {
                                setCursorId(title.id);
                                if (isDeletable(title)) toggle(title.id);
                              }}
                              className="grid aspect-square place-items-center"
                            >
                              <span className="relative">
                                <span
                                  className={cn(
                                    "rounded-sm",
                                    focused && "bg-[#4880c0] p-1",
                                    on && "ring-[3px] ring-[#73d98c]",
                                  )}
                                >
                                  <GameIcon id={parent?.id ?? title.id} />
                                </span>
                                {on ? <Picked /> : null}
                              </span>
                            </button>
                          );
                        })}
                      </div>
                    </div>
                  ))
                )}
              </div>
            ) : (
              <ul className="min-h-0 flex-1 overflow-auto" aria-label={shelf === "nds" ? "NDS/DSi" : "3DS"}>
                {visible.length === 0 ? (
                  <li className="py-6 text-sm text-muted">
                    {shelf === "nds" ? "No NDS/DSi software." : "No 3DS software."}
                  </li>
                ) : (
                  groupBlocks(visible, shelf).map((group) => (
                    <li key={group.label || "nds"}>
                      {group.label ? <p className="px-1 pt-2 text-xs text-muted">{group.label}</p> : null}
                      <ul>
                        {group.items.map((title) => {
                          const parent = parentOf(title, installed);
                          const name = parent?.name ?? title.name;
                          const on = selected.includes(title.id);
                          const focused = cursor?.id === title.id;
                          return (
                            <li key={title.id} id={`title-${title.id}`}>
                              <button
                                type="button"
                                onClick={() => {
                                  setCursorId(title.id);
                                  if (isDeletable(title)) toggle(title.id);
                                }}
                                className={cn("flex w-full items-center gap-2 px-1 py-1 text-left", focused && "bg-[#4880c0]")}
                              >
                                <span className="relative">
                                  <GameIcon id={parent?.id ?? title.id} />
                                  {on ? <Picked /> : null}
                                </span>
                                <span className="min-w-0 flex-1">
                                  <span className={cn("block truncate text-sm", on && "text-[#73d98c]")}>
                                    {clipChars(name, 14)}
                                  </span>
                                  <span className="block truncate text-xs text-muted">
                                    {clipChars(title.publisher || "-", 14)}
                                  </span>
                                </span>
                                <span className="shrink-0 text-xs tabular-nums">{consoleBytes(title.bytes)}</span>
                              </button>
                            </li>
                          );
                        })}
                      </ul>
                    </li>
                  ))
                )}
              </ul>
            )
          ) : grid ? (
            <div className="grid flex-1 grid-cols-4 content-start gap-1 overflow-auto">
              {cias.map((item, index) => (
                <button
                  key={item.id}
                  id={`cia-${item.id}`}
                  type="button"
                  aria-label={item.name}
                  onClick={() => {
                    setCiaCursor(index);
                    setCiaSelected((current) =>
                      current.includes(item.id) ? current.filter((id) => id !== item.id) : [...current, item.id],
                    );
                  }}
                  className="grid aspect-square place-items-center"
                >
                  <span className="relative">
                    <span
                      className={cn(
                        "rounded-sm",
                        index === ciaCursor && "bg-[#4880c0] p-1",
                        ciaSelected.includes(item.id) && "ring-[3px] ring-[#73d98c]",
                      )}
                    >
                      <GameIcon id={item.iconId} />
                    </span>
                    {ciaSelected.includes(item.id) ? <Picked /> : null}
                  </span>
                </button>
              ))}
            </div>
          ) : (
            <ul className="min-h-0 flex-1 overflow-auto" aria-label="CIAs">
              {cias.map((item, index) => (
                <li key={item.id} id={`cia-${item.id}`}>
                  <button
                    type="button"
                    onClick={() => {
                      setCiaCursor(index);
                      setCiaSelected((current) =>
                        current.includes(item.id) ? current.filter((id) => id !== item.id) : [...current, item.id],
                      );
                    }}
                    className={cn("flex w-full items-center gap-2 px-1 py-1 text-left", index === ciaCursor && "bg-[#4880c0]")}
                  >
                    <GameIcon id={item.iconId} />
                    <span className="min-w-0 flex-1">
                      <span className={cn("block truncate text-sm", ciaSelected.includes(item.id) && "text-[#73d98c]")}>
                        {clipChars(item.name, 14)}
                      </span>
                      <span className="block truncate text-xs text-muted">{clipChars(item.publisher, 14)}</span>
                    </span>
                    <span className="shrink-0 text-xs tabular-nums">{consoleBytes(item.bytes)}</span>
                  </button>
                </li>
              ))}
            </ul>
          )}
        </Screen>

        <section className="rounded-xl border border-border bg-surface p-4 text-sm text-muted">
          <p>
            Sample of cinder-{VERSION}. The 3DS shelf keeps Games, Updates, and DLC, then Homebrew. A title is
            licensed when its publisher matches a known first- or third-party 3DS developer.
          </p>
          <label className="mt-3 flex items-start gap-3">
            <Checkbox checked={includeRelated} onCheckedChange={(value) => setIncludeRelated(value === true)} />
            <span>Also delete the update and DLC that belong to a selected game.</span>
          </label>
          <p className="mt-3 font-mono text-xs text-subtle tabular-nums">
            {planned.length} queued · {consoleBytes(plannedBytes)} · freed {consoleBytes(freed)}
          </p>
          <div className="mt-3 flex flex-wrap gap-2">
            <Button variant="danger" disabled={planned.length === 0 || dialog === "working"} onClick={() => setDialog("confirm")}>
              Delete
            </Button>
            <Button variant="ghost" onClick={() => setMode((current) => (current === "install" ? "titles" : "install"))}>
              {mode === "install" ? "Titles" : "Install"}
            </Button>
            <Button variant="ghost" onClick={restoreLibrary}>
              <RotateCcw className="size-4" aria-hidden="true" />
              Restore sample
            </Button>
          </div>
        </section>
      </div>

      <AlertDialog.Root
        open={dialog !== "closed"}
        onOpenChange={(open) => {
          if (!open && dialog !== "working") setDialog("closed");
        }}
      >
        <AlertDialog.Portal>
          <AlertDialog.Overlay className="fixed inset-0 bg-bg/80" />
          <AlertDialog.Content className="fixed top-1/2 left-1/2 w-full max-w-sm -translate-x-1/2 -translate-y-1/2 rounded-xl border border-border bg-surface p-4">
            {dialog === "done" ? (
              <>
                <AlertDialog.Title className="text-lg font-medium">Removed</AlertDialog.Title>
                <AlertDialog.Description className="mt-2 text-sm text-muted">
                  Freed this session {consoleBytes(freed)}. The list reloads after this prompt.
                </AlertDialog.Description>
                <div className="mt-4 flex justify-end">
                  <AlertDialog.Cancel asChild>
                    <Button variant="primary">Back</Button>
                  </AlertDialog.Cancel>
                </div>
              </>
            ) : (
              <>
                <AlertDialog.Title className="text-lg font-medium">Delete {planned.length} titles?</AlertDialog.Title>
                <AlertDialog.Description className="mt-2 text-sm text-muted">
                  Size {consoleBytes(plannedBytes)}. This sample does not touch a console.
                </AlertDialog.Description>
                <ul className="mt-3 max-h-40 overflow-auto text-sm">
                  {planned.map((title) => (
                    <li key={title.id} className="flex items-center justify-between gap-3 py-1">
                      <span className="truncate">{parentOf(title, installed)?.name ?? title.name}</span>
                      <span className="shrink-0 text-xs text-subtle">{kindLabel(title.kind)}</span>
                    </li>
                  ))}
                </ul>
                <div className="mt-4 flex justify-end gap-2">
                  <AlertDialog.Cancel asChild>
                    <Button variant="ghost" disabled={dialog === "working"}>
                      Cancel
                    </Button>
                  </AlertDialog.Cancel>
                  <Button variant="danger" disabled={dialog === "working"} onClick={runDelete}>
                    {dialog === "working" ? "Deleting" : "Delete"}
                  </Button>
                </div>
              </>
            )}
          </AlertDialog.Content>
        </AlertDialog.Portal>
      </AlertDialog.Root>
    </main>
  );
}

function Screen({ wide, children }: { wide?: boolean; children: ReactNode }) {
  return (
    <section
      className={cn(
        "relative mx-auto flex w-full flex-col overflow-hidden rounded-md border border-border bg-[#0c0d0f] p-3",
        wide ? "aspect-[5/3] max-w-[640px]" : "aspect-[4/3] max-w-[512px]",
      )}
    >
      {children}
    </section>
  );
}

function Prompts({ items }: { items: [string, string][] }) {
  return (
    <div className="mt-auto flex flex-wrap items-center gap-x-3 gap-y-1 pt-2 text-[11px] text-muted">
      {items.map(([key, action]) => (
        <span key={action} className="inline-flex items-center gap-1">
          <KeyCap label={key} />
          {action}
        </span>
      ))}
    </div>
  );
}

function KeyCap({ label }: { label: string }) {
  const face = label === "A" || label === "X" || label === "Y";
  return (
    <span
      className={cn(
        "inline-grid place-items-center border border-[#c5ced8] text-[10px] text-fg",
        face ? "size-4 rounded-full" : "h-4 rounded-full px-1.5",
      )}
    >
      {label}
    </span>
  );
}

function Battery() {
  return (
    <span className="inline-flex items-center" aria-hidden="true">
      <span className="relative h-2.5 w-4 rounded-[2px] border border-fg">
        <span className="absolute inset-y-0.5 left-0.5 w-2.5 bg-fg" />
      </span>
      <span className="ml-px h-1.5 w-0.5 bg-fg" />
    </span>
  );
}

function GridMark() {
  return (
    <svg viewBox="0 0 18 18" className="size-4" aria-hidden="true">
      <rect x="1" y="1" width="7" height="7" rx="1" className="fill-muted" />
      <rect x="10" y="1" width="7" height="7" rx="1" className="fill-muted" />
      <rect x="1" y="10" width="7" height="7" rx="1" className="fill-muted" />
      <rect x="10" y="10" width="7" height="7" rx="1" className="fill-muted" />
    </svg>
  );
}

function ListMark() {
  return (
    <svg viewBox="0 0 18 18" className="size-4" aria-hidden="true">
      <rect x="1" y="2" width="16" height="2.2" rx="1" className="fill-muted" />
      <rect x="1" y="8" width="16" height="2.2" rx="1" className="fill-muted" />
      <rect x="1" y="14" width="16" height="2.2" rx="1" className="fill-muted" />
    </svg>
  );
}

function ShelfButton({
  current,
  value,
  count,
  onSelect,
  label,
}: {
  current: Shelf;
  value: Shelf;
  count: number;
  onSelect: (value: Shelf) => void;
  label: string;
}) {
  const active = current === value;
  return (
    <button
      type="button"
      role="tab"
      aria-selected={active}
      onClick={() => onSelect(value)}
      className={cn("press h-7 rounded-sm px-2 text-xs", active ? "bg-accent text-accent-fg" : "text-muted")}
    >
      {label}
      <span className="ml-1 tabular-nums">{count}</span>
    </button>
  );
}

type Ink = "accent" | "danger" | "ok" | "fg" | "muted";

const FILL: Record<Ink, string> = {
  accent: "fill-accent",
  danger: "fill-danger",
  ok: "fill-ok",
  fg: "fill-fg",
  muted: "fill-muted",
};

type Shape = { ink: Ink; d?: string; cx?: number; cy?: number; r?: number };

const STAMPS: Record<string, Shape[]> = {
  "0004000000A11000": [
    { ink: "accent", d: "M3 16h18v3H3zm5-1V7h8v8h-2V9h-4v6z" },
    { ink: "danger", d: "M10 3h4v2h-4zm1 2h2v3h-2z" },
  ],
  "0004000E00A11000": [{ ink: "ok", d: "M12 3 19 12h-4v9H9v-9H5z" }],
  "0004008C00A11000": [
    { ink: "accent", d: "M3 18h18v2H3z" },
    { ink: "danger", d: "M5 9h3v7H5zm5.5-4h3v11h-3zm5.5 5h3v6h-3z" },
  ],
  "0004000200A11000": [{ ink: "muted", d: "M6 3h2v18H6zm2 1h11l-3.5 4L19 12H8z" }],
  "0004000000A12000": [
    { ink: "accent", d: "M12 2 22 12 12 22 2 12z" },
    { ink: "danger", d: "M12 8 16 12 12 16 8 12z" },
  ],
  "0004008C00A12000": [
    { ink: "accent", d: "M4 8h16v3H4zm0 5h16v3H4z" },
    { ink: "danger", d: "M8 5h3v14H8zm5 0h3v14h-3z" },
  ],
  "0004000000A13000": [
    { ink: "accent", d: "M4 20V11h3v9zm6.5 0V6h3v14zM17 20v-8h3v8z" },
    { ink: "ok", d: "M4 8h3v2H4zm6.5-4h3v2h-3z" },
  ],
  "0004000E00A13000": [
    { ink: "ok", d: "M4 5h16v3H4zm0 5h12v3H4zm0 5h8v3H4z" },
    { ink: "accent", d: "M16 14h4v5h-4z" },
  ],
  "0004008C00A13000": [
    { ink: "accent", d: "M4 4h7v7H4zm9 0h7v7h-7zM4 13h7v7H4z" },
    { ink: "danger", d: "M13 13h7v7h-7z" },
  ],
  "0004000000A14000": [
    { ink: "ok", d: "M11 3h2v4h-2z" },
    { ink: "accent", cx: 12, cy: 14, r: 6 },
  ],
  "0004000000A15000": [
    { ink: "accent", d: "M2 14h20l-3 6H5z" },
    { ink: "fg", d: "M11 4h2v10h-2z" },
    { ink: "danger", d: "M7 7h10v2H7z" },
  ],
  "0004000000A16000": [
    { ink: "danger", d: "M3 10 12 3l9 7v2H3z" },
    { ink: "accent", d: "M5 12h14v9H5z" },
    { ink: "fg", d: "M8 15h3v2H8zm5 0h3v2h-3zM8 18h3v3H8z" },
  ],
  "0004000000A17000": [
    { ink: "accent", d: "M11 2h2v20h-2zM2 11h20v2H2z" },
    { ink: "danger", cx: 12, cy: 12, r: 3 },
  ],
  "0004000E00A17000": [
    { ink: "accent", d: "M4 5h9l7 7v9H4z" },
    { ink: "ok", d: "M13 5v7h7z" },
  ],
  "0004000000B22000": [
    { ink: "fg", d: "M6 3h12v18H6z" },
    { ink: "accent", d: "M6 7h12v2H6zm0 4h12v2H6zm0 4h8v2H6z" },
  ],
  "0004000000C1D400": [
    { ink: "danger", d: "M6 5h12a2 2 0 0 1 2 2v4H4V7a2 2 0 0 1 2-2z" },
    { ink: "accent", d: "M4 13h16v4a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2z" },
    { ink: "fg", d: "M14 2h2v3h-2z" },
  ],
  "000480044B4C4D00": [
    { ink: "accent", d: "M4 4h16v16H4z" },
    { ink: "danger", d: "M4 11h16v2H4zM11 4h2v16h-2z" },
  ],
  "000480044B4E5400": [
    { ink: "danger", d: "M3 11 12 4l9 7v2H3z" },
    { ink: "accent", d: "M6 13h12v7H6z" },
    { ink: "fg", d: "M11 15h2v5h-2z" },
  ],
  "0004003000008F02": [
    { ink: "fg", d: "M4 4h16v16H4z" },
    { ink: "accent", d: "M4 11h16v2H4zM11 11h2v9h-2z" },
  ],
  "0004001000021000": [
    { ink: "fg", d: "M3 7h18v2H3zm0 8h18v2H3z" },
    { ink: "accent", cx: 8, cy: 8, r: 2.4 },
    { ink: "danger", cx: 16, cy: 16, r: 2.4 },
  ],
  "0004001000020100": [
    { ink: "accent", d: "M11 3h2v9h-2zm-4 7h10l-5 6z" },
    { ink: "danger", d: "M5 19h14v2H5z" },
  ],
  "0004013000001C02": [
    { ink: "ok", cx: 7, cy: 12, r: 3.2 },
    { ink: "accent", cx: 17, cy: 12, r: 3.2 },
    { ink: "fg", d: "M10 11h4v2h-4z" },
  ],
};

function Picked() {
  return (
    <span className="absolute -top-1 -right-1 grid size-4 place-items-center rounded-full bg-[#73d98c] text-[10px] font-medium text-[#0c0d0f]">
      ✓
    </span>
  );
}

function GameIcon({ id, large = false }: { id: string; large?: boolean }) {
  const shapes = STAMPS[id] ?? [
    { ink: "accent" as const, d: "M4 4h16v16H4z" },
    { ink: "danger" as const, d: "M9 9h6v6H9z" },
  ];
  return (
    <span
      className={cn(
        "grid shrink-0 place-items-center rounded-md border border-border bg-bg",
        large ? "size-14" : "size-9",
      )}
      aria-hidden="true"
    >
      <svg viewBox="0 0 24 24" className={large ? "size-10" : "size-6"}>
        {shapes.map((shape, index) =>
          shape.d ? (
            <path key={index} d={shape.d} className={FILL[shape.ink]} />
          ) : (
            <circle key={index} cx={shape.cx} cy={shape.cy} r={shape.r} className={FILL[shape.ink]} />
          ),
        )}
      </svg>
    </span>
  );
}

import React, { useState, useEffect } from "react";

// Lightweight zero-dep Markdown Parser
function parseInline(text) {
  return text
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/\$O\(1\)\$/g, "O(1)")
    .replace(/\*\*(.*?)\*\*/g, "<strong>$1</strong>")
    .replace(/\*(.*?)\*/g, "<em>$1</em>")
    .replace(/`([^`]+)`/g, '<code class="md-inline-code">$1</code>')
    .replace(/\[([^\]]+)\]\(([^)]+)\)/g, '<a href="$2" class="md-link">$1</a>');
}

function renderMarkdown(md) {
  // Normalize Windows CRLF (\r\n) and legacy CR (\r) to standard LF (\n)
  md = (md || "").replace(/\r\n/g, "\n").replace(/\r/g, "\n");

  const codeBlocks = [];
  md = md.replace(/```([a-z0-9_-]*)\n([\s\S]*?)```/g, (match, lang, code) => {
    const placeholder = `__CODE_BLOCK_${codeBlocks.length}__`;
    const escaped = code
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;");
    codeBlocks.push(`<pre class="code-block" data-lang="${lang}"><code>${escaped}</code></pre>`);
    return placeholder;
  });

  const lines = md.split("\n");
  const html = [];
  let inTable = false;
  let inList = false;

  for (let i = 0; i < lines.length; i++) {
    let line = lines[i];

    if (line.trim().startsWith("__CODE_BLOCK_")) {
      if (inList) { html.push("</ul>"); inList = false; }
      if (inTable) { html.push("</tbody></table></div>"); inTable = false; }
      const idx = parseInt(line.trim().replace("__CODE_BLOCK_", "").replace("__", ""), 10);
      html.push(codeBlocks[idx]);
      continue;
    }

    if (/^---+\s*$/.test(line)) {
      if (inList) { html.push("</ul>"); inList = false; }
      if (inTable) { html.push("</tbody></table></div>"); inTable = false; }
      html.push('<hr class="md-hr" />');
      continue;
    }

    if (/^#{1,6}\s+/.test(line)) {
      if (inList) { html.push("</ul>"); inList = false; }
      if (inTable) { html.push("</tbody></table></div>"); inTable = false; }
      const level = line.match(/^#+/)[0].length;
      const text = parseInline(line.replace(/^#+\s+/, ""));
      html.push(`<h${level} class="md-h${level}">${text}</h${level}>`);
      continue;
    }

    if (line.trim().startsWith("|") && line.trim().endsWith("|")) {
      if (inList) { html.push("</ul>"); inList = false; }
      const cells = line.split("|").slice(1, -1).map((c) => c.trim());
      if (cells.every((c) => /^:?-+:?$/.test(c))) {
        continue;
      }

      if (!inTable) {
        inTable = true;
        html.push('<div class="table-scroll"><table class="md-table"><thead><tr>');
        cells.forEach((c) => html.push(`<th>${parseInline(c)}</th>`));
        html.push("</tr></thead><tbody>");
      } else {
        html.push("<tr>");
        cells.forEach((c) => html.push(`<td>${parseInline(c)}</td>`));
        html.push("</tr>");
      }
      continue;
    } else if (inTable) {
      html.push("</tbody></table></div>");
      inTable = false;
    }

    if (/^\s*[-*]\s+/.test(line)) {
      if (!inList) {
        html.push('<ul class="md-list">');
        inList = true;
      }
      const itemText = parseInline(line.replace(/^\s*[-*]\s+/, ""));
      html.push(`<li>${itemText}</li>`);
      continue;
    } else if (inList && line.trim() === "") {
      html.push("</ul>");
      inList = false;
    }

    if (line.trim() === "") continue;

    html.push(`<p class="md-p">${parseInline(line)}</p>`);
  }

  if (inList) html.push("</ul>");
  if (inTable) html.push("</tbody></table></div>");

  return html.join("\n");
}

// In-memory module cache so switching tabs is instant (0 ms, no freeze)
let cachedHtml = "";
let cachedStats = "";
let isFetching = false;

export default function DocsViewer() {
  const [htmlContent, setHtmlContent] = useState(cachedHtml);
  const [loadStats, setLoadStats] = useState(cachedStats || "STREAMING VIA VYOM.FS...");

  useEffect(() => {
    // If already cached, instant mount without re-reading or re-parsing!
    if (cachedHtml) {
      return;
    }

    if (isFetching) return;
    isFetching = true;

    async function loadDocs() {
      try {
        let filePath = "docs/API.md";
        if (window.vyom?.fs?.fileInfo) {
          const info = await window.vyom.fs.fileInfo(filePath);
          if (!info?.exists) {
            filePath = "../docs/API.md";
          }
        }

        const t0 = performance.now();
        let offset = 0;
        let md = "";

        while (true) {
          const chunk = await window.vyom.fs.readChunk(filePath, offset, 65536);
          md += chunk.data;
          if (chunk.eof || chunk.length === 0) break;
          offset += chunk.length;
        }

        const elapsed = (performance.now() - t0).toFixed(2);
        
        // Defer rendering slightly to let the UI finish paint/transition smoothly
        requestAnimationFrame(() => {
          const rendered = renderMarkdown(md);
          const stats = `STREAMED ${md.length} BYTES FROM DISK IN ${elapsed} MS (CACHED)`;
          cachedHtml = rendered;
          cachedStats = stats;
          setHtmlContent(rendered);
          setLoadStats(stats);
          isFetching = false;
        });
      } catch (err) {
        isFetching = false;
        setHtmlContent(
          `<div class="code-block" style="color: var(--danger-crimson);">
            Failed to stream docs/API.md: ${err.message || err}
          </div>`
        );
        setLoadStats("STREAM FAULT");
      }
    }

    loadDocs();
  }, []);

  return (
    <div className="docs-view">
      <div className="docs-badge-bar">
        <div className="docs-badge">
          <span className="dot"></span>
          <span>{loadStats}</span>
        </div>
      </div>

      <article
        className="docs-article"
        dangerouslySetInnerHTML={{ __html: htmlContent }}
      />
    </div>
  );
}


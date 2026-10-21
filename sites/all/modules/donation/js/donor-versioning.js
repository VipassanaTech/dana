/**
 * Dana donor versioning, revision 3 -- donation form side (new file sites/all/modules/donation/js/donor-versioning.js).
 * ES5 only. Needs jQuery >= 1.7 (sites run 1.10 via jquery_update) and jquery-confirm 3.3 (js/jquery-confirm.min.js).
 * Settings (Drupal.settings.danaDonor): cur_fy, today (DB clock), freeze (0/1), is_edit, orig_donor, orig_name, fycounts {fy: n}.
 * Server contract: /get-donor-detail?did=&fy= returns R2 keys + baseline, baseline_sig, cur_fy, baseline_fy.
 */
var DanaDonorCore = {
  GROUPS: { name: ["d_name"], gender: ["d_gender"], contact: ["d_contact"], email: ["d_email"],
            id: ["d_id_type", "d_id_no"], location: ["d_address", "d_pincode", "d_country", "d_state", "d_city"] },
  DV: { d_name: "dv_name", d_gender: "dv_gender", d_contact: "dv_contact", d_email: "dv_email", d_id_type: "dv_id_type",
        d_id_no: "dv_id_no", d_address: "dv_address", d_pincode: "dv_pincode", d_country: "dv_country", d_state: "dv_state", d_city: "dv_city" },
  LABEL: { d_name: "Name", d_gender: "Gender", d_contact: "Contact", d_email: "Email", d_id_type: "ID Type", d_id_no: "ID No",
           d_address: "Address", d_pincode: "Pincode", d_country: "Country", d_state: "State", d_city: "City" },
  /* same rule as donation_fy_from_date(); unpadded dates accepted */
  fyOf: function (date) {
    var m = /^\s*(\d{4})-(\d{1,2})-(\d{1,2})/.exec(date || "");
    if (!m) return 0;
    var y = +m[1], mo = +m[2];
    if (y === 0 || mo < 1 || mo > 12) return 0;
    return (mo >= 4) ? y : y - 1;
  },
  fyLabel: function (fy) { return fy + "-" + String(fy + 1).slice(-2); },
  /* mirror of donor_display_from_row(): 0 with an empty number shows as 99 (L21) */
  display: function (dv) {
    var o = {}, k, t = parseInt(dv.dv_id_type, 10) || 0;
    for (k in this.DV) o[k] = (dv[this.DV[k]] === null || dv[this.DV[k]] === undefined) ? "" : String(dv[this.DV[k]]);
    if (t === 0 && o.d_id_no === "") t = 99;
    o.d_id_type = String(t);
    return o;
  },
  /* mirror of donor_form_same() */
  norm: function (k, v) {
    v = String(v === null || v === undefined ? "" : v).replace(/\r\n/g, "\n").replace(/^[ \t\n\r\0\x0B]+|[ \t\n\r\0\x0B]+$/g, "");   // exactly PHP trim()'s characters
    return (k === "d_country" || k === "d_state" || k === "d_city") ? v.toLowerCase() : v;
  },
  /* mirror of donor_id_key() exact mode */
  idKey: function (t, n) {
    t = parseInt(t, 10) || 0;
    n = (n === null || n === undefined) ? "" : String(n);
    if (n === "" && (t === 0 || t === 99)) t = 99;
    return t + "|" + n;
  },
  groupSame: function (g, a, b) {
    if (g === "id") return this.idKey(a.d_id_type, a.d_id_no) === this.idKey(b.d_id_type, b.d_id_no);
    for (var i = 0; i < this.GROUPS[g].length; i++) {
      var k = this.GROUPS[g][i];
      if (this.norm(k, a[k]) !== this.norm(k, b[k])) return false;
    }
    return true;
  },
  changed: function (posted, shownDv) {
    var s = this.display(shownDv), out = [], g;
    for (g in this.GROUPS) if (!this.groupSame(g, posted, s)) out.push(g);
    return out;
  },
  /* M6: default from data -- a changed group the year holds differently from the current details -> "fy" */
  defaultScope: function (b, groups) {
    var s = this.display(b.fyv), l = this.display(b.live);
    for (var i = 0; i < groups.length; i++) if (!this.groupSame(groups[i], s, l)) return "fy";
    return "current";
  }
};

var DanaDonor = (function ($, C) {
  "use strict";
  var pending = 0;
  function S() { return (window.Drupal && Drupal.settings && Drupal.settings.danaDonor) || {}; }
  function esc(s) { return $("<div/>").text(s === null || s === undefined ? "" : String(s)).html(); }
  function get(k) { return (k === "d_gender") ? ($("input[name=d_gender]:checked").val() || "") : ($("[name=" + k + "]").val() || ""); }
  function set(k, v) {
    if (k === "d_gender") { $("input[name=d_gender]").prop("checked", false); $("input[name=d_gender]").filter(function () { return this.value === v; }).prop("checked", true); return; }
    if (k === "d_id_type" && v !== "" && !$("#edit-d-id-type option").filter(function () { return this.value === v; }).length)
      $("#edit-d-id-type").append($("<option/>").val(v).text("UNKNOWN (" + v + ")"));     // L20/L21: keep an unknown type unchanged
    $("[name=" + k + "]").val(v);
  }
  function posted() { var o = {}, k; for (k in C.DV) o[k] = get(k); return o; }
  function baseline() { var j = $("input[name=donor_baseline]").val(); if (!j) return null; try { return $.parseJSON(j); } catch (e) { return null; } }
  function typeText(v) { var t = $("#edit-d-id-type option").filter(function () { return this.value === String(v); }).text(); return t || ("UNKNOWN (" + v + ")"); }
  function shown(k, v) { return (k === "d_id_type") ? typeText(v) : v; }

  function apply(r, keep) {                      // fill from the SIGNED baseline, so shown == baseline by construction
    var b = $.parseJSON(r.baseline), d = C.display(b.fyv), g, i;
    for (g in C.GROUPS) {
      if ($.inArray(g, keep) >= 0) continue;     // refetch: groups the clerk changed keep the typed text
      for (i = 0; i < C.GROUPS[g].length; i++) set(C.GROUPS[g][i], d[C.GROUPS[g][i]]);
    }
    $("input[name=d_donor]").val(r.did);
    $("input[name=donor_baseline]").val(r.baseline);
    $("input[name=donor_baseline_sig]").val(r.baseline_sig);
    if (r.cur_fy) { S().cur_fy = r.cur_fy; $("input[name=form_cur_fy]").val(r.cur_fy); }
    S().fycounts = r.fycounts || {};
    badge(b, r.fy_since);
  }
  /* L28 badge (read-path spec 6.1). ONE source: the signed baseline (on page load via DanaDonor.badge(), and after
     every pick/refetch), so the badge always describes exactly the values shown and posted. */
  function badge(b, since) {
    if (typeof window.donation_render_fy_badge !== "function") return;
    if (!b) { window.donation_render_fy_badge({ fy: 0 }); return; }
    window.donation_render_fy_badge({ fy: b.fy, cur_fy: S().cur_fy, frozen: b.own, live: C.display(b.live), shown: C.display(b.fyv),
      fy_since: since || (b.since ? b.since.split("-").reverse().join("/") : "") });
  }
  function load(did, fy, keep) {
    pending++;
    return $.getJSON("/get-donor-detail", { did: did, fy: fy }).done(function (data) {
      if (!data || !data.length || !data[0].baseline) {
        alert("Could not load this donor's details. Please pick the donor again.");
        if (!keep) clear();
        return;
      }
      apply(data[0], keep || []);
    }).fail(function () {
      alert("Could not load the donor's details (network error). Please try again.");
      if (!keep) clear();
    }).always(function () { pending--; });
  }
  /* L29 pick_donor: overwrite EVERY donor field; never keep typed text (switching donors carries nothing over) */
  function pick(did) { load(did, C.fyOf($("#edit-d-date").val()) || S().cur_fy, null); }
  /* L29 refetch_donor_fy: same donor, the date moved to another FY; keep the groups the clerk changed */
  function refetch() {
    var b = baseline(), did = parseInt($("input[name=d_donor]").val(), 10), fy = C.fyOf($("#edit-d-date").val());
    if (!b || !(did > 0) || did !== b.donor || !fy || fy === b.fy) return;
    load(did, fy, C.changed(posted(), b.fyv));
  }
  function clear() {                             // name cleared: new donor (case "-1 without baseline")
    $("input[name=d_donor]").val("-1");
    $("input[name=donor_baseline]").val("");
    $("input[name=donor_baseline_sig]").val("");
    $.each(["d_country", "d_state", "d_city", "d_id_no", "d_contact", "d_pincode", "d_address", "d_email"], function (i, k) { $("[name=" + k + "]").val(""); });
    badge(null);
  }
  function n(fy) { var c = (S().fycounts || {})[fy]; return c ? +c : 0; }
  function others(exceptFy) {
    var out = [];
    $.each(S().fycounts || {}, function (fy, c) { fy = +fy; if (fy > 0 && fy < S().cur_fy && fy !== exceptFy) out.push("FY " + C.fyLabel(fy) + ": " + c); });
    return out.join(", ");
  }
  function table(b, groups, past) {
    var p = posted(), s = C.display(b.fyv), l = C.display(b.live);
    var h = "<table class=\"donor-changes\"><tr><th>Field</th><th>" + (past ? "FY " + C.fyLabel(b.fy) + " (shown)" : "Before") + "</th><th>New</th>"
      + (past ? "<th>Current details</th>" : "") + "</tr>";
    $.each(groups, function (i, g) {
      $.each(C.GROUPS[g], function (j, k) {
        h += "<tr><td>" + C.LABEL[k] + "</td><td>" + esc(shown(k, s[k])) + "</td><td><b>" + esc(shown(k, p[k])) + "</b></td>"
          + (past ? "<td>" + esc(shown(k, l[k])) + "</td>" : "") + "</tr>";
      });
    });
    return h + "</table>";
  }
  function liveChanges(b, groups) {              // M4: make visible what an option does to the CURRENT details
    var p = posted(), l = C.display(b.live), out = [];
    $.each(groups, function (i, g) {
      if (C.groupSame(g, p, l)) return;
      $.each(C.GROUPS[g], function (j, k) {
        if (C.norm(k, p[k]) !== C.norm(k, l[k])) out.push(C.LABEL[k] + " \"" + esc(shown(k, l[k])) + "\" &rarr; \"" + esc(shown(k, p[k])) + "\"");
      });
    });
    return out.length ? "<br><small>Current details change: " + out.join("; ") + "</small>" : "";
  }
  function scopeHtml(b, groups, fy) {
    var L = C.fyLabel(fy), def = C.defaultScope(b, groups), lc = liveChanges(b, groups), o = others(fy), h;
    function opt(v, text) { return "<p><label><input type=\"radio\" name=\"scope_choice\" value=\"" + v + "\"" + (v === def ? " checked=\"checked\"" : "") + "> " + text + "</label></p>"; }
    h = "<p><b>This donation is dated in FY " + L + ". How should the donor changes be applied?</b></p>";
    h += opt("current", "<b>From now on</b>: change the current details AND FY " + L + " (" + n(fy) + " receipt(s) of FY " + L + ", and all future receipts)." + lc);
    h += opt("fy", "<b>Only FY " + L + "</b>: the donor's details were different in that year. The current details are NOT changed.");
    if (S().freeze)
      h += opt("live", "<b>Current details only</b>: FY " + L + " keeps its own details, so THIS receipt is printed and sent with FY " + L + "'s details (the 'FY " + L + " (shown)' column)." + lc);
    if (o) h += "<p><small>Other financial years keep their own details (" + o + ").</small></p>";
    if (S().today > (fy + 1) + "-05-31")
      h += "<p class=\"error\">Form 10BD for FY " + L + " (due 31 May " + (fy + 1) + ") has probably been filed. 'From now on' and 'Only FY " + L + "' change that year's donor details; a revised 10BD may be needed.</p>";
    else
      h += "<p><small>FY " + L + "'s Form 10BD is due on 31 May " + (fy + 1) + ". 'From now on' and 'Only FY " + L + "' put this change into it" + (S().freeze ? "; 'Current details only' does not." : ".") + "</small></p>";
    return h;
  }
  function currentHtml() {
    var o = others(0);
    return "<p>" + (o ? "Earlier financial years (" + o + ") keep the details they have now. " : "")
      + "This year's donations (" + n(S().cur_fy) + ") show the new details. Receipts already issued are not changed; after saving you will be offered to reprint them.</p>";
  }
  /* replaces confirm_form() + donor_change_check(): ONE jquery-confirm dialog; the choice goes into the hidden
     donor_scope field INSIDE the donation form (M5; the cancel-dialog pattern), then the form is submitted */
  function confirm(form, summary) {
    if (pending > 0) { alert("Donor details are still loading. Please wait a moment and submit again."); $("input[type=submit]").prop("disabled", false); return false; }
    var b = baseline(), did = String($("input[name=d_donor]").val()), fy = C.fyOf($("#edit-d-date").val()), need = false, h = summary, groups, past;
    if (S().is_edit && did !== String(S().orig_donor))
      h += "<p><b>This donation is being reassigned</b> from \"" + esc(S().orig_name) + "\" to \"" + esc(get("d_name")) + "\".</p>";
    if (did !== "-1" && b) {
      groups = C.changed(posted(), b.fyv);
      if (groups.length) {
        past = fy > 0 && fy < S().cur_fy;
        h += "<h4>Donor details changed</h4>" + table(b, groups, past);
        if (past) { need = true; h += scopeHtml(b, groups, fy); } else h += currentHtml();
      }
    }
    $.confirm({
      title: "Confirm Details", content: h, columnClass: "large",
      buttons: {
        ok: { text: "Confirm", btnClass: "btn-blue", action: function () {
          var s = "";
          if (need) {
            s = this.$content.find("input[name=scope_choice]:checked").val() || "";
            if (!s) { $.alert("Please choose how to apply the donor changes."); return false; }
          }
          $("input[name=donor_scope]").val(s);
          form.submit();                          // native submit: the jQuery submit handler does not run again
        } },
        cancel: { text: "Cancel", action: function () { $("input[name=donor_scope]").val(""); $("input[type=submit]").prop("disabled", false); } }
      }
    });
    return false;
  }
  return { pick: pick, refetch: refetch, clear: clear, confirm: confirm, esc: esc, pending: function () { return pending > 0; },
           badge: function () { badge(baseline()); } };
})(jQuery, DanaDonorCore);

/* L28 badge. Ships appended to sites/all/modules/donation/js/donor-versioning.js (one file); fed only from the
   signed baseline (DanaDonor.badge() on load, apply() after pick/refetch). FY rule = DanaDonorCore.fyOf. */
(function ($) {
  function fy_label(fy) { return fy + "-" + String(fy + 1).slice(-2); }
  var badge_fields = { d_name: "#edit-d-name", d_contact: "#edit-d-contact", d_email: "#edit-d-email", d_id_no: "#edit-d-id-no",
    d_address: "#edit-d-address", d_pincode: "#edit-d-pincode", d_country: "#edit-d-country", d_state: "#edit-d-state", d_city: "#edit-d-city" };
  function norm(k, v) {   // exact (register N2: one rule, no case/space folding); only CRLF -> LF like the form compare
    return String(v == null ? "" : v).replace(/\r\n/g, "\n");
  }
  // info = {fy, cur_fy, frozen, fy_since, live:{d_name..}, shown:{d_name..}}
  window.donation_render_fy_badge = function (info) {
    var $b = $("#donor-fy-badge");
    $.each(badge_fields, function (k, sel) { $(sel).removeClass("fy-differs").removeAttr("title"); });
    if (!info || !info.fy || info.fy >= info.cur_fy) { $b.hide().text(""); return; }
    if (!info.frozen) { $b.text("FY " + fy_label(info.fy) + " uses the current donor details.").show(); return; }
    var diff = [];
    $.each(badge_fields, function (k, sel) {
      if (info.live && info.shown && norm(k, info.live[k]) !== norm(k, info.shown[k])) {
        $(sel).addClass("fy-differs").attr("title", "Current record: " + (info.live[k] || "(blank)"));
        diff.push($("label[for=" + sel.substr(1) + "]").text().replace("*", "").trim());
      }
    });
    $b.text("FY " + fy_label(info.fy) + " uses its own donor details (since " + info.fy_since + ")."
      + (diff.length ? " Different from the current record: " + diff.join(", ") + "." : "")).show();
  };
})(jQuery);

(function() {
    function aa() {
        return function() {}
    }

    function ba(a) {
        return function(b) {
            this[a] = b
        }
    }

    function h(a) {
        return function() {
            return this[a]
        }
    }

    function ca(a) {
        return function() {
            return a
        }
    }
    for (var q, fa = "function" == typeof Object.defineProperties ? Object.defineProperty : function(a, b, c) {
            if (c.get || c.set) throw new TypeError("ES3 does not support getters and setters.");
            a != Array.prototype && a != Object.prototype && (a[b] = c.value)
        }, ga = "undefined" != typeof window && window === this ? this : "undefined" != typeof global && null != global ? global : this, ha = ["Array", "prototype", "fill"], ia = 0; ia < ha.length - 1; ia++) {
        var ja = ha[ia];
        ja in ga || (ga[ja] = {});
        ga = ga[ja]
    }
    var ka = ha[ha.length - 1],
        la = ga[ka],
        ma = la ? la : function(a, b, c) {
            var d = this.length || 0;
            0 > b && (b = Math.max(0, d + b));
            if (null == c || c > d) c = d;
            c = Number(c);
            0 > c && (c = Math.max(0, d + c));
            for (b = Number(b || 0); b < c; b++) this[b] = a;
            return this
        };
    ma != la && null != ma && fa(ga, ka, {
        configurable: !0,
        writable: !0,
        value: ma
    });
    var oa = oa || {},
        w = this;

    function y(a) {
        return void 0 !== a
    }

    function pa(a) {
        a = a.split(".");
        for (var b = w, c; c = a.shift();)
            if (null != b[c]) b = b[c];
            else return null;
        return b
    }

    function C() {}

    function ra(a) {
        a.Xa = void 0;
        a.hg = function() {
            return a.Xa ? a.Xa : a.Xa = new a
        }
    }

    function ta(a) {
        var b = typeof a;
        if ("object" == b)
            if (a) {
                if (a instanceof Array) return "array";
                if (a instanceof Object) return b;
                var c = Object.prototype.toString.call(a);
                if ("[object Window]" == c) return "object";
                if ("[object Array]" == c || "number" == typeof a.length && "undefined" != typeof a.splice && "undefined" != typeof a.propertyIsEnumerable && !a.propertyIsEnumerable("splice")) return "array";
                if ("[object Function]" == c || "undefined" != typeof a.call && "undefined" != typeof a.propertyIsEnumerable && !a.propertyIsEnumerable("call")) return "function"
            } else return "null";
        else if ("function" == b && "undefined" == typeof a.call) return "object";
        return b
    }

    function ua(a) {
        return "array" == ta(a)
    }

    function va(a) {
        var b = ta(a);
        return "array" == b || "object" == b && "number" == typeof a.length
    }

    function wa(a) {
        return "string" == typeof a
    }

    function xa(a) {
        return "number" == typeof a
    }

    function ya(a) {
        return "function" == ta(a)
    }

    function za(a) {
        var b = typeof a;
        return "object" == b && null != a || "function" == b
    }

    function Aa(a) {
        return a[Ba] || (a[Ba] = ++Ca)
    }
    var Ba = "closure_uid_" + (1E9 * Math.random() >>> 0),
        Ca = 0;

    function Da(a, b, c) {
        return a.call.apply(a.bind, arguments)
    }

    function Ea(a, b, c) {
        if (!a) throw Error();
        if (2 < arguments.length) {
            var d = Array.prototype.slice.call(arguments, 2);
            return function() {
                var c = Array.prototype.slice.call(arguments);
                Array.prototype.unshift.apply(c, d);
                return a.apply(b, c)
            }
        }
        return function() {
            return a.apply(b, arguments)
        }
    }

    function E(a, b, c) {
        E = Function.prototype.bind && -1 != Function.prototype.bind.toString().indexOf("native code") ? Da : Ea;
        return E.apply(null, arguments)
    }

    function Fa(a, b) {
        var c = Array.prototype.slice.call(arguments, 1);
        return function() {
            var b = c.slice();
            b.push.apply(b, arguments);
            return a.apply(this, b)
        }
    }

    function F() {
        return +new Date
    }

    function Ga(a, b) {
        a = a.split(".");
        var c = w;
        a[0] in c || !c.execScript || c.execScript("var " + a[0]);
        for (var d; a.length && (d = a.shift());) !a.length && y(b) ? c[d] = b : c[d] && Object.prototype.hasOwnProperty.call(c, d) ? c = c[d] : c = c[d] = {}
    }

    function H(a, b) {
        function c() {}
        c.prototype = b.prototype;
        a.V = b.prototype;
        a.prototype = new c;
        a.prototype.constructor = a;
        a.hn = function(a, c, f) {
            for (var d = Array(arguments.length - 2), e = 2; e < arguments.length; e++) d[e - 2] = arguments[e];
            return b.prototype[c].apply(a, d)
        }
    };

    function Ha(a) {
        if (Error.captureStackTrace) Error.captureStackTrace(this, Ha);
        else {
            var b = Error().stack;
            b && (this.stack = b)
        }
        a && (this.message = String(a))
    }
    H(Ha, Error);
    Ha.prototype.name = "CustomError";
    var Ia;

    function Ja(a, b) {
        var c = a.length - b.length;
        return 0 <= c && a.indexOf(b, c) == c
    }

    function La(a) {
        return a.replace(/^[\s\xa0]+|[\s\xa0]+$/g, "")
    }

    function Ma(a) {
        if (!Na.test(a)) return a; - 1 != a.indexOf("&") && (a = a.replace(Oa, "&amp;")); - 1 != a.indexOf("<") && (a = a.replace(Pa, "&lt;")); - 1 != a.indexOf(">") && (a = a.replace(Qa, "&gt;")); - 1 != a.indexOf('"') && (a = a.replace(Sa, "&quot;")); - 1 != a.indexOf("'") && (a = a.replace(Ta, "&#39;")); - 1 != a.indexOf("\x00") && (a = a.replace(Ua, "&#0;"));
        return a
    }
    var Oa = /&/g,
        Pa = /</g,
        Qa = />/g,
        Sa = /"/g,
        Ta = /'/g,
        Ua = /\x00/g,
        Na = /[\x00&<>"']/;

    function Va(a) {
        return -1 != a.indexOf("&") ? "document" in w ? Wa(a) : Xa(a) : a
    }

    function Wa(a) {
        var b = {
                "&amp;": "&",
                "&lt;": "<",
                "&gt;": ">",
                "&quot;": '"'
            },
            c;
        c = w.document.createElement("div");
        return a.replace(Ya, function(a, e) {
            var d = b[a];
            if (d) return d;
            "#" == e.charAt(0) && (e = Number("0" + e.substr(1)), isNaN(e) || (d = String.fromCharCode(e)));
            d || (c.innerHTML = a + " ", d = c.firstChild.nodeValue.slice(0, -1));
            return b[a] = d
        })
    }

    function Xa(a) {
        return a.replace(/&([^;]+);/g, function(a, c) {
            switch (c) {
                case "amp":
                    return "&";
                case "lt":
                    return "<";
                case "gt":
                    return ">";
                case "quot":
                    return '"';
                default:
                    return "#" != c.charAt(0) || (c = Number("0" + c.substr(1)), isNaN(c)) ? a : String.fromCharCode(c)
            }
        })
    }
    var Ya = /&([^;\s<&]+);?/g;

    function Za() {
        return -1 != $a.toLowerCase().indexOf("webkit")
    }
    var ab = String.prototype.repeat ? function(a, b) {
        return a.repeat(b)
    } : function(a, b) {
        return Array(b + 1).join(a)
    };

    function bb(a, b) {
        a = y(void 0) ? a.toFixed(void 0) : String(a);
        var c = a.indexOf("."); - 1 == c && (c = a.length);
        return ab("0", Math.max(0, b - c)) + a
    }

    function db(a, b) {
        var c = 0;
        a = La(String(a)).split(".");
        b = La(String(b)).split(".");
        for (var d = Math.max(a.length, b.length), e = 0; 0 == c && e < d; e++) {
            var f = a[e] || "",
                g = b[e] || "";
            do {
                f = /(\d*)(\D*)(.*)/.exec(f) || ["", "", "", ""];
                g = /(\d*)(\D*)(.*)/.exec(g) || ["", "", "", ""];
                if (0 == f[0].length && 0 == g[0].length) break;
                c = eb(0 == f[1].length ? 0 : parseInt(f[1], 10), 0 == g[1].length ? 0 : parseInt(g[1], 10)) || eb(0 == f[2].length, 0 == g[2].length) || eb(f[2], g[2]);
                f = f[3];
                g = g[3]
            } while (0 == c)
        }
        return c
    }

    function eb(a, b) {
        return a < b ? -1 : a > b ? 1 : 0
    }
    var fb = 2147483648 * Math.random() | 0;

    function hb(a) {
        return String(a).replace(/\-([a-z])/g, function(a, c) {
            return c.toUpperCase()
        })
    }

    function jb(a) {
        var b = wa(void 0) ? "undefined".replace(/([-()\[\]{}+?*.$\^|,:#<!\\])/g, "\\$1").replace(/\x08/g, "\\x08") : "\\s";
        return a.replace(new RegExp("(^" + (b ? "|[" + b + "]+" : "") + ")([a-z])", "g"), function(a, b, e) {
            return b + e.toUpperCase()
        })
    };

    function kb() {};

    function lb(a) {
        return a[a.length - 1]
    }

    function mb(a, b, c) {
        c = null == c ? 0 : 0 > c ? Math.max(0, a.length + c) : c;
        if (wa(a)) return wa(b) && 1 == b.length ? a.indexOf(b, c) : -1;
        for (; c < a.length; c++)
            if (c in a && a[c] === b) return c;
        return -1
    }

    function nb(a, b, c) {
        for (var d = a.length, e = wa(a) ? a.split("") : a, f = 0; f < d; f++) f in e && b.call(c, e[f], f, a)
    }

    function ob(a, b) {
        for (var c = a.length, d = Array(c), e = wa(a) ? a.split("") : a, f = 0; f < c; f++) f in e && (d[f] = b.call(void 0, e[f], f, a));
        return d
    }

    function pb(a, b) {
        for (var c = a.length, d = wa(a) ? a.split("") : a, e = 0; e < c; e++)
            if (e in d && b.call(void 0, d[e], e, a)) return !0;
        return !1
    }

    function qb(a, b) {
        b = rb(a, b, void 0);
        return 0 > b ? null : wa(a) ? a.charAt(b) : a[b]
    }

    function rb(a, b, c) {
        for (var d = a.length, e = wa(a) ? a.split("") : a, f = 0; f < d; f++)
            if (f in e && b.call(c, e[f], f, a)) return f;
        return -1
    }

    function sb(a, b) {
        return 0 <= mb(a, b)
    }

    function tb(a, b) {
        b = mb(a, b);
        var c;
        (c = 0 <= b) && ub(a, b);
        return c
    }

    function ub(a, b) {
        Array.prototype.splice.call(a, b, 1)
    }

    function vb(a) {
        return Array.prototype.concat.apply(Array.prototype, arguments)
    }

    function wb(a) {
        var b = a.length;
        if (0 < b) {
            for (var c = Array(b), d = 0; d < b; d++) c[d] = a[d];
            return c
        }
        return []
    }

    function xb(a, b) {
        for (var c = 1; c < arguments.length; c++) {
            var d = arguments[c];
            if (va(d)) {
                var e = a.length || 0,
                    f = d.length || 0;
                a.length = e + f;
                for (var g = 0; g < f; g++) a[e + g] = d[g]
            } else a.push(d)
        }
    }

    function yb(a, b, c, d) {
        Array.prototype.splice.apply(a, zb(arguments, 1))
    }

    function zb(a, b, c) {
        return 2 >= arguments.length ? Array.prototype.slice.call(a, b) : Array.prototype.slice.call(a, b, c)
    }

    function Ab(a, b) {
        a.sort(b || Bb)
    }

    function Cb(a, b, c) {
        if (!va(a) || !va(b) || a.length != b.length) return !1;
        var d = a.length;
        c = c || Db;
        for (var e = 0; e < d; e++)
            if (!c(a[e], b[e])) return !1;
        return !0
    }

    function Bb(a, b) {
        return a > b ? 1 : a < b ? -1 : 0
    }

    function Db(a, b) {
        return a === b
    };

    function Eb(a, b, c) {
        return Math.min(Math.max(a, b), c)
    }

    function Fb(a, b) {
        a %= b;
        return 0 > a * b ? a + b : a
    }

    function Gb(a, b, c) {
        return a + c * (b - a)
    }

    function Ib(a) {
        return Fb(a, 360)
    }

    function Jb(a) {
        return a * Math.PI / 180
    }

    function Kb(a) {
        return 180 * a / Math.PI
    }

    function Lb(a, b) {
        a = Ib(b) - Ib(a);
        180 < a ? a -= 360 : -180 >= a && (a = 360 + a);
        return a
    };

    function Mb(a, b, c) {
        for (var d in a) b.call(c, a[d], d, a)
    }

    function Nb(a, b) {
        for (var c in a)
            if (b.call(void 0, a[c], c, a)) return !0;
        return !1
    }

    function Ob(a) {
        var b = [],
            c = 0,
            d;
        for (d in a) b[c++] = a[d];
        return b
    }

    function Pb(a) {
        var b = [],
            c = 0,
            d;
        for (d in a) b[c++] = d;
        return b
    }

    function Qb(a) {
        for (var b in a) return !1;
        return !0
    }
    var Rb = "constructor hasOwnProperty isPrototypeOf propertyIsEnumerable toLocaleString toString valueOf".split(" ");

    function Sb(a, b) {
        for (var c, d, e = 1; e < arguments.length; e++) {
            d = arguments[e];
            for (c in d) a[c] = d[c];
            for (var f = 0; f < Rb.length; f++) c = Rb[f], Object.prototype.hasOwnProperty.call(d, c) && (a[c] = d[c])
        }
    };

    function Tb(a) {
        Tb[" "](a);
        return a
    }
    Tb[" "] = C;

    function Ub(a, b) {
        var c = Vb;
        return Object.prototype.hasOwnProperty.call(c, a) ? c[a] : c[a] = b(a)
    };
    var $a;
    a: {
        var Wb = w.navigator;
        if (Wb) {
            var Xb = Wb.userAgent;
            if (Xb) {
                $a = Xb;
                break a
            }
        }
        $a = ""
    }

    function Yb(a) {
        return -1 != $a.indexOf(a)
    };

    function Zb() {
        return Yb("iPhone") && !Yb("iPod") && !Yb("iPad")
    };

    function $b(a) {
        var b = a;
        if (a instanceof Array) b = Array(a.length), ac(b, a);
        else if (a instanceof Object) {
            var c = b = {},
                d;
            for (d in a) a.hasOwnProperty(d) && (c[d] = $b(a[d]))
        }
        return b
    }

    function ac(a, b) {
        for (var c = 0; c < b.length; ++c) b.hasOwnProperty(c) && (a[c] = $b(b[c]))
    }

    function bc(a, b) {
        a !== b && (a.length = 0, b && (a.length = b.length, ac(a, b)))
    }

    function cc(a, b) {
        a[b] || (a[b] = []);
        return a[b]
    }

    function dc(a, b) {
        if (null == a || null == b) return null == a == (null == b);
        if (a.constructor != Array && a.constructor != Object) throw Error("Invalid object type passed into jsproto.areObjectsEqual()");
        if (a === b) return !0;
        if (a.constructor != b.constructor) return !1;
        for (var c in a)
            if (!(c in b && ec(a[c], b[c]))) return !1;
        for (var d in b)
            if (!(d in a)) return !1;
        return !0
    }

    function ec(a, b) {
        if (a === b || !(!0 !== a && 1 !== a || !0 !== b && 1 !== b) || !(!1 !== a && 0 !== a || !1 !== b && 0 !== b)) return !0;
        if (a instanceof Object && b instanceof Object) {
            if (!dc(a, b)) return !1
        } else return !1;
        return !0
    }

    function fc(a, b, c, d) {
        this.type = a;
        this.label = b;
        this.Bi = c;
        this.Ce = d
    }

    function gc(a) {
        switch (a) {
            case "d":
            case "f":
            case "i":
            case "j":
            case "u":
            case "v":
            case "x":
            case "y":
            case "g":
            case "h":
            case "n":
            case "o":
            case "e":
                return 0;
            case "s":
            case "z":
            case "B":
                return "";
            case "b":
                return !1;
            default:
                return null
        }
    }

    function hc(a, b, c) {
        b = y(b) ? b : gc(a);
        return new fc(a, 1, b, c)
    }

    function ic(a, b) {
        b = y(b) ? b : gc(a);
        return new fc(a, 2, b, void 0)
    }

    function jc(a, b, c) {
        return new fc(a, 3, c, b)
    }
    var kc = hc("d", void 0);
    ic("d", void 0);
    jc("d");
    var lc = hc("f", void 0);
    ic("f", void 0);
    jc("f");
    var mc = hc("i", void 0);
    ic("i", void 0);
    jc("i");
    hc("j", void 0);
    ic("j", void 0);
    jc("j", void 0, void 0);
    jc("j", void 0, "");
    hc("u", void 0);
    ic("u", void 0);
    jc("u");
    hc("v", void 0);
    ic("v", void 0);
    jc("v", void 0, void 0);
    jc("v", void 0, "");
    var nc = hc("b", void 0);
    ic("b", void 0);
    jc("b");

    function oc(a) {
        return hc("e", a)
    }
    var pc = oc();
    ic("e", void 0);
    var qc = jc("e"),
        rc = hc("s", void 0);
    ic("s", void 0);
    var tc = jc("s");
    hc("B", void 0);
    ic("B", void 0);
    jc("B");

    function I(a, b) {
        return hc("m", a, b)
    }

    function uc(a) {
        return jc("m", a)
    }
    var vc = hc("x", void 0),
        yc = ic("x", void 0);
    jc("x");
    hc("y", void 0);
    ic("y", void 0);
    jc("y");
    hc("g", void 0);
    ic("g", void 0);
    jc("g");
    hc("h", void 0);
    ic("h", void 0);
    jc("h");
    hc("n", void 0);
    ic("n", void 0);
    jc("n");
    hc("o", void 0);
    ic("o", void 0);
    jc("o", void 0, void 0);
    jc("o", void 0, "");

    function zc() {
        return Yb("Trident") || Yb("MSIE")
    }

    function Ac() {
        return (Yb("Chrome") || Yb("CriOS")) && !Yb("Edge")
    };

    function Bc(a) {
        this.data = a || {}
    }

    function Cc(a, b, c) {
        a = a.data[b];
        return null != a ? a : c
    }

    function Dc(a, b) {
        return Cc(a, b, "")
    }

    function Ec(a) {
        var b = {};
        cc(a.data, "param").push(b);
        return b
    }

    function Fc(a, b) {
        return cc(a.data, "param")[b]
    }

    function Gc(a) {
        return a.data.param ? a.data.param.length : 0
    };

    function J(a) {
        this.data = a || []
    }

    function K(a, b) {
        return null != a.data[b]
    }

    function Hc(a, b, c) {
        a = a.data[b];
        return null != a ? a : c
    }

    function Ic(a, b, c) {
        return !!Hc(a, b, c)
    }

    function L(a, b, c) {
        return Hc(a, b, c || 0)
    }

    function N(a, b, c) {
        return Hc(a, b, c || 0)
    }

    function O(a, b, c) {
        return Hc(a, b, c || "")
    }

    function P(a, b) {
        var c = a.data[b];
        c || (c = a.data[b] = []);
        return c
    }

    function Jc(a, b) {
        b in a.data && delete a.data[b]
    }

    function Kc(a, b, c) {
        cc(a.data, b).push(c)
    }

    function Lc(a, b, c) {
        return cc(a.data, b)[c]
    }

    function Mc(a, b) {
        var c = [];
        cc(a.data, b).push(c);
        return c
    }

    function Nc(a, b, c) {
        return cc(a.data, b)[c]
    }

    function S(a, b) {
        return a.data[b] ? a.data[b].length : 0
    }

    function Oc(a, b) {
        return dc(a.data, b ? b.data : null)
    }
    J.prototype.ib = h("data");

    function Pc(a) {
        var b = [];
        bc(b, a.ib());
        return b
    }

    function U(a, b) {
        bc(a.data, b ? b.ib() : null)
    };
    var Qc = Yb("Opera"),
        Rc = zc(),
        Sc = Yb("Edge"),
        Tc = Yb("Gecko") && !(Za() && !Yb("Edge")) && !(Yb("Trident") || Yb("MSIE")) && !Yb("Edge"),
        Uc = Za() && !Yb("Edge"),
        Vc = Uc && Yb("Mobile"),
        Wc = Yb("Macintosh"),
        Xc = Yb("Windows"),
        Yc = Yb("Linux") || Yb("CrOS");

    function Zc() {
        var a = w.document;
        return a ? a.documentMode : void 0
    }
    var $c;
    a: {
        var ad = "",
            bd = function() {
                var a = $a;
                if (Tc) return /rv\:([^\);]+)(\)|;)/.exec(a);
                if (Sc) return /Edge\/([\d\.]+)/.exec(a);
                if (Rc) return /\b(?:MSIE|rv)[: ]([^\);]+)(\)|;)/.exec(a);
                if (Uc) return /WebKit\/(\S+)/.exec(a);
                if (Qc) return /(?:Version)[ \/]?(\S+)/.exec(a)
            }();bd && (ad = bd ? bd[1] : "");
        if (Rc) {
            var cd = Zc();
            if (null != cd && cd > parseFloat(ad)) {
                $c = String(cd);
                break a
            }
        }
        $c = ad
    }
    var dd = $c,
        Vb = {};

    function ed(a) {
        return Ub(a, function() {
            return 0 <= db(dd, a)
        })
    }
    var fd;
    var gd = w.document;
    fd = gd && Rc ? Zc() || ("CSS1Compat" == gd.compatMode ? parseInt(dd, 10) : 5) : void 0;

    function hd(a) {
        this.data = a || []
    }
    var id;
    H(hd, J);

    function jd(a) {
        this.data = a || []
    }
    var kd;
    H(jd, J);

    function ld(a) {
        this.data = a || []
    }
    var md;
    H(ld, J);

    function nd() {
        md || (md = {
            A: -1,
            R: [, mc, mc]
        });
        return md
    }
    ld.prototype.W = function() {
        return N(this, 1)
    };

    function od(a) {
        this.data = a || []
    }
    var pd;
    H(od, J);

    function qd() {
        pd || (pd = {
            A: -1,
            R: [, rc, rc, rc]
        });
        return pd
    };

    function rd(a) {
        this.data = a || []
    }
    var sd;
    H(rd, J);

    function td(a) {
        this.data = a || []
    }
    var ud;
    H(td, J);

    function vd(a) {
        this.data = a || []
    }
    var wd;
    H(vd, J);

    function xd(a) {
        this.data = a || []
    }
    var yd;
    H(xd, J);

    function zd(a) {
        this.data = a || []
    }
    var Ad;
    H(zd, J);

    function Bd(a) {
        this.data = a || []
    }
    var Cd;
    H(Bd, J);

    function Dd(a) {
        this.data = a || []
    }
    H(Dd, J);

    function Ed(a, b) {
        a.data[0] = b
    };
    var Fd = Yb("Firefox"),
        Gd = Zb() || Yb("iPod"),
        Hd = Yb("iPad"),
        Id = Yb("Android") && !(Ac() || Yb("Firefox") || Yb("Opera") || Yb("Silk")),
        Jd = Ac(),
        Kd = Yb("Safari") && !(Ac() || Yb("Coast") || Yb("Opera") || Yb("Edge") || Yb("Silk") || Yb("Android")) && !(Zb() || Yb("iPad") || Yb("iPod"));

    function Ld(a) {
        this.data = a || []
    }
    H(Ld, J);

    function Md(a) {
        this.data = a || []
    }
    H(Md, J);

    function Nd(a, b) {
        a.data[0] = b
    }

    function Od(a, b) {
        a.data[1] = b
    }
    Md.prototype.Ja = function() {
        return O(this, 2)
    };

    function Pd(a) {
        this.data = a || []
    }
    var Qd;
    H(Pd, J);

    function Rd(a) {
        this.data = a || []
    }
    var Sd;
    H(Rd, J);

    function Td(a) {
        this.data = a || []
    }
    var Vd;
    H(Td, J);
    var Wd = null,
        Xd = null,
        Yd = null;

    function Zd(a) {
        var b = [];
        $d(a, function(a) {
            b.push(a)
        });
        return b
    }

    function $d(a, b) {
        function c(b) {
            for (; d < a.length;) {
                var c = a.charAt(d++),
                    e = Xd[c];
                if (null != e) return e;
                if (!/^[\s\xa0]*$/.test(c)) throw Error("Unknown base64 encoding at char: " + c);
            }
            return b
        }
        ae();
        for (var d = 0;;) {
            var e = c(-1),
                f = c(0),
                g = c(64),
                k = c(64);
            if (64 === k && -1 === e) break;
            b(e << 2 | f >> 4);
            64 != g && (b(f << 4 & 240 | g >> 2), 64 != k && b(g << 6 & 192 | k))
        }
    }

    function ae() {
        if (!Wd) {
            Wd = {};
            Xd = {};
            Yd = {};
            for (var a = 0; 65 > a; a++) Wd[a] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=".charAt(a), Xd[Wd[a]] = a, Yd[a] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.".charAt(a), 62 <= a && (Xd["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.".charAt(a)] = a)
        }
    };

    function be(a) {
        this.data = a || []
    }
    var ce;
    H(be, J);

    function de(a) {
        this.data = a || []
    }
    var ee;
    H(de, J);

    function fe() {
        ee || (ee = {
            A: -1,
            R: [, rc, rc]
        });
        return ee
    }

    function ge(a) {
        return O(a, 0)
    };

    function he(a) {
        this.data = a || []
    }
    var ie;
    H(he, J);

    function le(a) {
        this.data = a || []
    }
    H(le, J);

    function me(a) {
        this.data = a || []
    }
    H(me, J);

    function ne(a) {
        this.data = a || []
    }
    H(ne, J);

    function oe(a) {
        this.data = a || []
    }
    H(oe, J);

    function pe(a) {
        return N(a, 3)
    }

    function qe(a) {
        return new me(a.data[0])
    }

    function re(a) {
        return new me(P(a, 0))
    }

    function se(a) {
        return new ne(a.data[1])
    }

    function te(a) {
        return new ne(P(a, 1))
    }

    function ue(a) {
        return new oe(a.data[2])
    }

    function ve(a) {
        return new oe(P(a, 2))
    }

    function we(a) {
        return N(a, 1)
    }

    function xe(a) {
        return N(a, 2)
    }

    function ye(a) {
        return N(a, 0)
    }

    function ze(a, b) {
        a.data[0] = b
    }

    function Ae(a) {
        return N(a, 0)
    }
    oe.prototype.W = function() {
        return N(this, 0)
    };

    function Be(a, b) {
        a.data[0] = b
    }

    function Ce(a) {
        return N(a, 1)
    }

    function De(a, b) {
        a.data[1] = b
    };

    function Ee(a) {
        this.data = a || []
    }
    H(Ee, J);

    function Fe(a) {
        this.data = a || []
    }
    var Ge;
    H(Fe, J);

    function He() {
        Ge || (Ge = {
            A: -1,
            R: []
        });
        return Ge
    };

    function Ie(a) {
        this.data = a || []
    }
    H(Ie, J);

    function Je(a) {
        this.data = a || []
    }
    H(Je, J);

    function Ke(a) {
        return new de(a.data[0])
    };

    function Le(a) {
        this.data = a || []
    }
    H(Le, J);

    function Me(a) {
        this.data = a || []
    }
    H(Me, J);

    function Ne(a) {
        this.data = a || []
    }
    var Oe;
    H(Ne, J);

    function Pe() {
        Oe || (Oe = {
            A: -1,
            R: [, pc, rc]
        });
        return Oe
    }

    function Qe(a, b) {
        a.data[0] = b
    }
    Ne.prototype.za = function() {
        return O(this, 1)
    };

    function Re(a, b) {
        a.data[1] = b
    };

    function Se(a) {
        this.data = a || []
    }
    H(Se, J);

    function Te(a) {
        this.data = a || []
    }
    H(Te, J);

    function Ue(a) {
        this.data = a || []
    }
    var Ve;
    H(Ue, J);

    function We(a) {
        this.data = a || []
    }
    H(We, J);

    function Xe(a) {
        this.data = a || []
    }
    H(Xe, J);

    function Ye(a) {
        this.data = a || []
    }
    var Ze;
    H(Ye, J);

    function $e(a) {
        return new Ue(a.data[0])
    }

    function af() {
        Ve || (Ve = {
            A: -1,
            R: []
        }, Ve.R = [, , , kc, kc, oc(1)]);
        return Ve
    }

    function bf(a, b) {
        a.data[2] = b
    }

    function cf(a, b) {
        a.data[3] = b
    };

    function df(a) {
        this.data = a || []
    }
    H(df, J);

    function ef(a) {
        this.data = a || []
    }
    H(ef, J);

    function ff(a) {
        this.data = a || []
    }
    H(ff, J);

    function gf(a) {
        this.data = a || []
    }
    H(gf, J);

    function hf(a) {
        this.data = a || []
    }
    H(hf, J);

    function jf(a) {
        return new ef(a.data[3])
    }
    df.prototype.eb = function() {
        return new hf(this.data[8])
    };

    function kf(a) {
        this.data = a || []
    }
    var lf;
    H(kf, J);

    function mf(a) {
        this.data = a || []
    }
    var nf;
    H(mf, J);

    function of(a) {
        this.data = a || []
    }
    var pf;
    H(of, J);

    function qf(a) {
        this.data = a || []
    }
    var rf;
    H(qf, J);

    function sf(a) {
        this.data = a || []
    }
    var tf;
    H(sf, J);

    function uf() {
        if (!tf) {
            var a = tf = {
                    A: -1,
                    R: []
                },
                b = oc(1),
                c = hc("j", ""),
                d = new mf([]);
            nf || (nf = {
                A: -1,
                R: [, nc]
            });
            var d = I(d, nf),
                e = oc(2),
                f = new qf([]);
            if (!rf) {
                var g = rf = {
                        A: -1,
                        R: []
                    },
                    k = new of([]);
                pf || (pf = {
                    A: -1,
                    R: [, , nc]
                });
                g.R = [, I(k, pf)]
            }
            f = I(f, rf);
            g = new Bd([]);
            Cd || (Cd = {
                A: -1,
                R: []
            }, Cd.R = [, hc("b", !0), hc("b", !0), hc("b", !0), hc("b", !0), nc, tc]);
            g = I(g, Cd);
            k = new vd([]);
            if (!wd) {
                var l = wd = {
                        A: -1,
                        R: []
                    },
                    m = new rd([]);
                sd || (sd = {
                    A: -1,
                    R: []
                }, sd.R = [, nc, hc("b", !0)]);
                var m = I(m, sd),
                    n = new td([]);
                ud || (ud = {
                    A: -1,
                    R: [, nc]
                });
                l.R = [, m, I(n, ud)]
            }
            k = I(k,
                wd);
            l = new jd([]);
            kd || (kd = {
                A: -1,
                R: [, , nc, nc, nc]
            });
            l = I(l, kd);
            m = new he([]);
            ie || (ie = {
                A: -1,
                R: []
            }, ie.R = [, ic("j", ""), yc, yc]);
            a.R = [, rc, b, c, pc, rc, d, e, f, rc, g, k, l, I(m, ie)]
        }
        return tf
    }

    function vf(a, b) {
        a.data[0] = b
    };

    function wf(a) {
        this.data = a || []
    }
    H(wf, J);
    wf.prototype.rc = function() {
        return N(this, 0)
    };

    function xf(a) {
        this.data = a || []
    }
    var yf;
    H(xf, J);

    function zf() {
        yf || (yf = {
            A: -1,
            R: []
        }, yf.R = [, ic("y", ""), ic("y", ""), I(new Fe([]), He())]);
        return yf
    };

    function Af(a) {
        this.data = a || []
    }
    H(Af, J);

    function Bf(a) {
        this.data = a || []
    }
    H(Bf, J);

    function Cf(a) {
        this.data = a || []
    }
    var Df;
    H(Cf, J);

    function Ef() {
        if (!Df) {
            var a = Df = {
                A: -1,
                R: []
            };
            lf || (lf = {
                A: -1,
                R: [, pc, nc, pc, pc]
            });
            a.R = [, uc(lf), nc, I(new ld([]), nd()), nc]
        }
        return Df
    };

    function Ff(a) {
        this.data = a || []
    }
    var Gf;
    H(Ff, J);

    function Hf(a) {
        this.data = a || []
    }
    var If;
    H(Hf, J);

    function Jf(a) {
        this.data = a || []
    }
    var Kf;
    H(Jf, J);

    function Lf() {
        If || (If = {
            A: -1,
            R: [, vc, vc]
        });
        return If
    }

    function Mf() {
        if (!Kf) {
            var a = Kf = {
                    A: -1,
                    R: []
                },
                b = I(new xf([]), zf()),
                c = hc("v", ""),
                d = new Ff([]);
            Gf || (Gf = {
                A: -1,
                R: []
            }, Gf.R = [, uc(Lf()), uc(zf()), uc(zf()), I(new Hf([]), Lf()), nc, mc]);
            a.R = [, b, rc, c, I(d, Gf), rc, rc]
        }
        return Kf
    }

    function Nf(a, b) {
        a.data[4] = b
    };

    function Of(a) {
        this.data = a || []
    }
    H(Of, J);

    function Pf(a) {
        this.data = a || []
    }
    var Qf;
    H(Pf, J);

    function Rf(a) {
        this.data = a || []
    }
    var Sf;
    H(Rf, J);

    function Tf() {
        Qf || (Qf = {
            A: -1,
            R: []
        }, Qf.R = [, I(new xf([]), zf()), uc(Uf()), uc(zf()), uc(zf()), I(new Rf([]), Uf())]);
        return Qf
    }

    function Vf(a, b) {
        var c = Tf();
        a = a.ib();
        var d = Wf,
            e = "!",
            f = b[0];
        if ("0" > f || "9" < f) b = b.substr(1), f != e && (e = f, d = Xf(e));
        b = b.split(e);
        a.length = 0;
        Yf(0, b.length, b, d, c, a)
    }

    function Uf() {
        Sf || (Sf = {
            A: -1,
            R: [, vc, vc]
        });
        return Sf
    };

    function Zf(a) {
        this.data = a || []
    }
    H(Zf, J);

    function $f(a) {
        this.data = a || []
    }
    var ag;
    H($f, J);
    $f.prototype.Pc = function() {
        return new Ue(this.data[0])
    };
    var bg;

    function cg(a) {
        this.data = a || []
    }
    var dg;
    H(cg, J);

    function eg(a) {
        this.data = a || []
    }
    H(eg, J);

    function fg(a) {
        this.data = a || []
    }
    H(fg, J);
    fg.prototype.ma = function() {
        return new eg(this.data[1])
    };

    function gg(a) {
        this.data = a || []
    }
    H(gg, J);

    function hg(a) {
        this.data = a || []
    }
    var ig;
    H(hg, J);

    function jg(a) {
        this.data = a || []
    }
    var qg;
    H(jg, J);

    function rg(a) {
        this.data = a || []
    }
    var sg;
    H(rg, J);

    function tg(a) {
        this.data = a || []
    }
    var ug;
    H(tg, J);

    function vg(a) {
        this.data = a || []
    }
    var wg;
    H(vg, J);

    function xg() {
        if (!qg) {
            var a = qg = {
                    A: -1,
                    R: []
                },
                b = new Pd([]);
            if (!Qd) {
                var c = Qd = {
                        A: -1,
                        R: []
                    },
                    d = new Rd([]);
                Sd || (Sd = {
                    A: -1,
                    R: [, qc]
                });
                var d = I(d, Sd),
                    e = new Td([]);
                Vd || (Vd = {
                    A: -1,
                    R: []
                }, Vd.R = [, , , oc(255), qc]);
                c.R = [, d, nc, nc, , nc, nc, I(e, Vd), nc, nc]
            }
            b = I(b, Qd);
            c = I(new od([]), qd());
            d = new rg([]);
            if (!sg) {
                var e = sg = {
                        A: -1,
                        R: []
                    },
                    f = new cg([]);
                if (!dg) {
                    var g = dg = {
                            A: -1,
                            R: []
                        },
                        k = uc(nd());
                    bg || (bg = {
                        A: -1,
                        R: []
                    }, bg.R = [, I(new ld([]), nd()), rc]);
                    g.R = [, k, nc, uc(bg)]
                }
                f = I(f, dg);
                g = new be([]);
                ce || (ce = {
                    A: -1,
                    R: [, pc, pc]
                });
                e.R = [, , f, , I(g, ce)]
            }
            d = I(d, sg);
            e = new vg([]);
            wg || (wg = {
                A: -1,
                R: []
            }, wg.R = [, mc, rc, hc("i", 100)]);
            e = I(e, wg);
            f = I(new Fe([]), He());
            g = new xd([]);
            yd || (yd = {
                A: -1,
                R: []
            }, yd.R = [, oc(1), hc("d", 6), hc("d", 2), hc("d", .04)]);
            var g = I(g, yd),
                k = I(new Cf([]), Ef()),
                l = new tg([]);
            ug || (ug = {
                A: -1,
                R: []
            }, ug.R = [, oc(1), nc]);
            var l = I(l, ug),
                m = new hd([]);
            id || (id = {
                A: -1,
                R: [, qc]
            });
            var m = I(m, id),
                n = new zd([]);
            Ad || (Ad = {
                A: -1,
                R: [, tc]
            });
            a.R = [, b, c, d, e, , , , f, g, nc, k, l, m, nc, pc, rc, mc, qc, I(n, Ad)]
        }
        return qg
    };

    function yg(a) {
        this.data = a || []
    }
    var zg;
    H(yg, J);

    function Ag(a) {
        this.data = a || []
    }
    H(Ag, J);

    function Bg(a) {
        this.data = a || []
    }
    H(Bg, J);

    function Cg(a) {
        this.data = a || []
    }
    var Dg;
    H(Cg, J);

    function Eg(a) {
        this.data = a || []
    }
    var Fg;
    H(Eg, J);

    function Gg(a) {
        this.data = a || []
    }
    H(Gg, J);

    function Hg(a) {
        this.data = a || []
    }
    H(Hg, J);

    function Ig(a) {
        this.data = a || []
    }
    H(Ig, J);

    function Jg(a) {
        this.data = a || []
    }
    H(Jg, J);

    function Kg(a) {
        this.data = a || []
    }
    H(Kg, J);

    function Lg(a) {
        this.data = a || []
    }
    var Mg;
    H(Lg, J);

    function Ng(a) {
        this.data = a || []
    }
    H(Ng, J);

    function Og(a) {
        this.data = a || []
    }
    H(Og, J);

    function Pg(a) {
        this.data = a || []
    }
    H(Pg, J);

    function Qg(a) {
        this.data = a || []
    }
    H(Qg, J);

    function Rg(a) {
        this.data = a || []
    }
    H(Rg, J);
    var Sg;

    function Tg(a) {
        return new Te(a.data[1])
    }
    Bg.prototype.getTime = function(a) {
        return new Jg(Nc(this, 8, a))
    };

    function Ug(a) {
        return new Ne(a.data[0])
    }
    Gg.prototype.fa = function() {
        return new Bf(this.data[1])
    };

    function Vg(a, b) {
        return new Gg(Nc(a, 0, b))
    };

    function Wg(a) {
        this.data = a || []
    }
    H(Wg, J);

    function Xg(a) {
        this.data = a || []
    }
    var Yg;
    H(Xg, J);

    function Zg(a) {
        this.data = a || []
    }
    H(Zg, J);

    function $g(a) {
        this.data = a || []
    }
    var ah;
    H($g, J);

    function bh(a) {
        this.data = a || []
    }
    var ch;
    H(bh, J);

    function dh(a) {
        this.data = a || []
    }
    var eh;
    H(dh, J);

    function fh(a) {
        this.data = a || []
    }
    H(fh, J);

    function gh(a) {
        this.data = a || []
    }
    H(gh, J);

    function hh(a) {
        this.data = a || []
    }
    var ih;
    H(hh, J);

    function jh(a) {
        this.data = a || []
    }
    H(jh, J);

    function kh(a) {
        this.data = a || []
    }
    var lh;
    H(kh, J);

    function mh(a) {
        this.data = a || []
    }
    H(mh, J);

    function nh() {
        if (!Yg) {
            var a = Yg = {
                    A: -1,
                    R: []
                },
                b = I(new sf([]), uf()),
                c = I(new od([]), qd());
            ah || (ah = {
                A: -1,
                R: []
            }, ah.R = [, I(new Ne([]), Pe()), I(new Jf([]), Mf())]);
            a.R = [, b, c, uc(ah), I(new bh([]), oh())]
        }
        return Yg
    }
    Xg.prototype.getContext = function() {
        return new sf(this.data[0])
    };
    Zg.prototype.Ea = function() {
        return new wf(this.data[0])
    };
    Zg.prototype.eb = function(a) {
        return new fh(Nc(this, 1, a))
    };

    function oh() {
        if (!ch) {
            var a = ch = {
                A: -1,
                R: []
            };
            Dg || (Dg = {
                A: -1,
                R: [, pc]
            });
            var b = uc(Dg),
                c = oc(2),
                d = new dh([]);
            eh || (eh = {
                A: -1,
                R: []
            }, eh.R = [, hc("i", 100)]);
            d = I(d, eh);
            Mg || (Mg = {
                A: -1,
                R: [, pc]
            });
            var e = uc(Mg);
            Fg || (Fg = {
                A: -1,
                R: [, pc]
            });
            var f = uc(Fg);
            Sg || (Sg = {
                A: -1,
                R: []
            }, Sg.R = [, oc(1), oc(1)]);
            a.R = [, qc, b, c, d, e, f, , uc(Sg), I(new Cf([]), Ef())]
        }
        return ch
    }
    fh.prototype.Ea = function() {
        return new gh(this.data[0])
    };

    function ph(a) {
        return new Ne(a.data[1])
    }

    function qh(a) {
        return new df(a.data[2])
    }

    function rh(a) {
        return new df(P(a, 2))
    }

    function sh(a) {
        return new Me(a.data[3])
    }

    function th(a) {
        return new Of(a.data[6])
    }

    function uh(a, b) {
        return new Bg(Nc(a, 5, b))
    }
    gh.prototype.rc = function() {
        return L(this, 0)
    };

    function vh() {
        if (!ih) {
            var a = ih = {
                    A: -1,
                    R: []
                },
                b = I(new sf([]), uf()),
                c = new $f([]);
            if (!ag) {
                var d = ag = {
                        A: -1,
                        R: []
                    },
                    e = I(new Ue([]), af()),
                    f = new Ye([]);
                Ze || (Ze = {
                    A: -1,
                    R: []
                }, Ze.R = [, hc("v", ""), lc, I(new de([]), fe()), I(new de([]), fe())]);
                d.R = [, e, kc, I(f, Ze), uc(zf()), I(new Ue([]), af()), rc]
            }
            var c = I(c, ag),
                d = I(new jg([]), xg()),
                e = I(new bh([]), oh()),
                f = I(new Ne([]), Pe()),
                g = new kh([]);
            lh || (lh = {
                A: -1,
                R: []
            });
            var g = I(g, lh),
                k = I(new Pf([]), Tf()),
                l = new yg([]);
            if (!zg) {
                var m = zg = {
                        A: -1,
                        R: []
                    },
                    n = uc(Mf()),
                    p = I(new jg([]), xg()),
                    r = new hg([]);
                ig || (ig = {
                    A: -1,
                    R: []
                }, ig.R = [, uc(xg()), hc("b", !0)]);
                m.R = [, n, p, rc, , I(r, ig)]
            }
            a.R = [, b, c, d, e, f, g, k, I(l, zg)]
        }
        return ih
    }
    hh.prototype.getContext = function() {
        return new sf(this.data[0])
    };
    jh.prototype.Ea = function() {
        return new wf(this.data[0])
    };
    jh.prototype.getMetadata = function() {
        return new fh(this.data[1])
    };
    jh.prototype.Uc = function() {
        return K(this, 3)
    };
    jh.prototype.tb = function() {
        return new mh(this.data[3])
    };

    function wh(a) {
        this.data = a || []
    }
    H(wh, J);

    function xh(a) {
        this.data = a || []
    }
    H(xh, J);

    function yh(a) {
        this.data = a || []
    }
    H(yh, J);
    wh.prototype.za = function() {
        return O(this, 0)
    };

    function zh(a) {
        return L(a, 2, 1)
    }
    wh.prototype.ia = function() {
        return new fh(this.data[21])
    };

    function Ah(a) {
        return new fh(P(a, 21))
    }
    wh.prototype.fa = function() {
        return new le(this.data[8])
    };

    function Bh(a) {
        return new le(P(a, 8))
    }

    function Nh(a, b) {
        Kc(a, 17, b)
    }

    function Oh(a) {
        return new yh(Nc(a, 15, 0))
    }

    function Ph(a) {
        return new Md(P(a, 1))
    };

    function Qh(a) {
        this.data = a || []
    }
    H(Qh, J);

    function Rh(a) {
        this.data = a || []
    }
    H(Rh, J);

    function Sh(a) {
        this.data = a || []
    }
    H(Sh, J);

    function Th(a) {
        this.data = a || []
    }
    H(Th, J);

    function Uh(a) {
        return new Rh(a.data[2])
    }

    function Vh(a) {
        return new Rh(P(a, 2))
    }

    function Wh(a) {
        return new Th(a.data[4])
    };

    function Xh() {
        this.A = new Qh;
        (new Wg(P(this.A, 5))).data[6] = 98
    }

    function Yh(a, b) {
        Vh(a.A).data[10] = b
    }

    function Zh(a, b) {
        a = Vh(a.A);
        Kc(a, 3, b)
    }

    function $h(a, b) {
        a = Vh(a.A);
        Kc(a, 9, b)
    }

    function ai(a, b) {
        Vh(a.A).data[0] = b
    }

    function bi(a, b) {
        Vh(a.A).data[8] = b
    }

    function ci(a, b) {
        (new Ee(P(a.A, 1))).data[0] = b
    }

    function di(a, b) {
        (new Ee(P(a.A, 1))).data[1] = b
    }

    function ei(a, b) {
        (new rd(P(new vd(P(a.A, 16)), 0))).data[0] = b
    };
    var fi = {
        area: !0,
        base: !0,
        br: !0,
        col: !0,
        command: !0,
        embed: !0,
        hr: !0,
        img: !0,
        input: !0,
        keygen: !0,
        link: !0,
        meta: !0,
        param: !0,
        source: !0,
        track: !0,
        wbr: !0
    };
    var gi = /<[^>]*>|&[^;]+;/g;

    function hi(a, b) {
        return b ? a.replace(gi, "") : a
    }
    var ii = /[A-Za-z\u00c0-\u00d6\u00d8-\u00f6\u00f8-\u02b8\u0300-\u0590\u0800-\u1fff\u200e\u2c00-\ufb1c\ufe00-\ufe6f\ufefd-\uffff]/,
        ji = /^[^A-Za-z\u00c0-\u00d6\u00d8-\u00f6\u00f8-\u02b8\u0300-\u0590\u0800-\u1fff\u200e\u2c00-\ufb1c\ufe00-\ufe6f\ufefd-\uffff]*[\u0591-\u06ef\u06fa-\u07ff\u200f\ufb1d-\ufdff\ufe70-\ufefc]/,
        ki = /^http:\/\/.*/,
        li = /[A-Za-z\u00c0-\u00d6\u00d8-\u00f6\u00f8-\u02b8\u0300-\u0590\u0800-\u1fff\u200e\u2c00-\ufb1c\ufe00-\ufe6f\ufefd-\uffff][^\u0591-\u06ef\u06fa-\u07ff\u200f\ufb1d-\ufdff\ufe70-\ufefc]*$/,
        mi =
        /[\u0591-\u06ef\u06fa-\u07ff\u200f\ufb1d-\ufdff\ufe70-\ufefc][^A-Za-z\u00c0-\u00d6\u00d8-\u00f6\u00f8-\u02b8\u0300-\u0590\u0800-\u1fff\u200e\u2c00-\ufb1c\ufe00-\ufe6f\ufefd-\uffff]*$/,
        ni = /\s+/,
        oi = /[\d\u06f0-\u06f9]/;

    function pi(a, b) {
        var c = 0,
            d = 0,
            e = !1;
        a = hi(a, b).split(ni);
        for (b = 0; b < a.length; b++) {
            var f = a[b];
            ji.test(hi(f, void 0)) ? (c++, d++) : ki.test(f) ? e = !0 : ii.test(hi(f, void 0)) ? d++ : oi.test(f) && (e = !0)
        }
        return 0 == d ? e ? 1 : 0 : .4 < c / d ? -1 : 1
    };

    function qi() {
        this.A = "";
        this.B = ri
    }
    qi.prototype.Yb = !0;
    qi.prototype.lb = h("A");
    qi.prototype.toString = function() {
        return "Const{" + this.A + "}"
    };

    function si(a) {
        return a instanceof qi && a.constructor === qi && a.B === ri ? a.A : "type_error:Const"
    }
    var ri = {};

    function ti(a) {
        var b = new qi;
        b.A = a;
        return b
    }
    ti("");

    function ui() {
        this.A = "";
        this.B = vi
    }
    ui.prototype.Yb = !0;
    var vi = {};
    ui.prototype.lb = h("A");

    function wi(a) {
        var b = new ui;
        b.A = a;
        return b
    }
    var xi = wi(""),
        yi = /^([-,."'%_!# a-zA-Z0-9]+|(?:rgb|hsl)a?\([0-9.%, ]+\))$/;

    function zi() {
        this.A = "";
        this.B = Ai
    }
    zi.prototype.Yb = !0;
    zi.prototype.lb = h("A");
    zi.prototype.Ke = !0;
    zi.prototype.uc = ca(1);

    function Bi(a) {
        if (a instanceof zi && a.constructor === zi && a.B === Ai) return a.A;
        ta(a);
        return "type_error:TrustedResourceUrl"
    }
    var Ai = {};

    function Ci() {
        this.A = "";
        this.B = Di
    }
    Ci.prototype.Yb = !0;
    Ci.prototype.lb = h("A");
    Ci.prototype.Ke = !0;
    Ci.prototype.uc = ca(1);

    function Ei(a) {
        if (a instanceof Ci && a.constructor === Ci && a.B === Di) return a.A;
        ta(a);
        return "type_error:SafeUrl"
    }
    var Fi = /^(?:(?:https?|mailto|ftp):|[^&:/?#]*(?:[/?#]|$))/i;

    function Gi(a) {
        if (a instanceof Ci) return a;
        a = a.Yb ? a.lb() : String(a);
        Fi.test(a) || (a = "about:invalid#zClosurez");
        return Hi(a)
    }
    var Di = {};

    function Hi(a) {
        var b = new Ci;
        b.A = a;
        return b
    }
    Hi("about:blank");

    function Ii() {
        this.A = "";
        this.C = Ji;
        this.B = null
    }
    Ii.prototype.Ke = !0;
    Ii.prototype.uc = h("B");
    Ii.prototype.Yb = !0;
    Ii.prototype.lb = h("A");

    function Ki(a) {
        if (a instanceof Ii && a.constructor === Ii && a.C === Ji) return a.A;
        ta(a);
        return "type_error:SafeHtml"
    }

    function Li(a) {
        if (a instanceof Ii) return a;
        var b = null;
        a.Ke && (b = a.uc());
        a = Ma(a.Yb ? a.lb() : String(a));
        return Mi(a, b)
    }

    function Ni(a) {
        if (a instanceof Ii) return a;
        a = Li(a);
        var b;
        b = Ki(a).replace(/  /g, " &#160;").replace(/(\r\n|\r|\n)/g, "<br>");
        return Mi(b, a.uc())
    }
    var Oi = /^[a-zA-Z0-9-]+$/,
        Pi = {
            action: !0,
            cite: !0,
            data: !0,
            formaction: !0,
            href: !0,
            manifest: !0,
            poster: !0,
            src: !0
        },
        Qi = {
            APPLET: !0,
            BASE: !0,
            EMBED: !0,
            IFRAME: !0,
            LINK: !0,
            MATH: !0,
            META: !0,
            OBJECT: !0,
            SCRIPT: !0,
            STYLE: !0,
            SVG: !0,
            TEMPLATE: !0
        };

    function Ri(a) {
        function b(a) {
            ua(a) ? nb(a, b) : (a = Li(a), d += Ki(a), a = a.uc(), 0 == c ? c = a : 0 != a && c != a && (c = null))
        }
        var c = 0,
            d = "";
        nb(arguments, b);
        return Mi(d, c)
    }
    var Ji = {};

    function Mi(a, b) {
        var c = new Ii;
        c.A = a;
        c.B = b;
        return c
    }
    Mi("<!DOCTYPE html>", 0);
    Mi("", 0);
    Mi("<br>", 0);

    function Si(a, b) {
        si(a);
        si(a);
        return Mi(b, null)
    };

    function Ti(a) {
        return function() {
            return a
        }
    }
    var Ui = Ti(!0);
    var Yi = "StopIteration" in w ? w.StopIteration : {
        message: "StopIteration",
        stack: ""
    };

    function Zi() {}
    Zi.prototype.next = function() {
        throw Yi;
    };
    Zi.prototype.qe = function() {
        return this
    };

    function $i(a, b) {
        this.B = {};
        this.A = [];
        this.C = this.ca = 0;
        var c = arguments.length;
        if (1 < c) {
            if (c % 2) throw Error("Uneven number of arguments");
            for (var d = 0; d < c; d += 2) this.set(arguments[d], arguments[d + 1])
        } else if (a) {
            a instanceof $i ? (c = a.kb(), d = a.Ca()) : (c = Pb(a), d = Ob(a));
            for (var e = 0; e < c.length; e++) this.set(c[e], d[e])
        }
    }
    q = $i.prototype;
    q.Bb = h("ca");
    q.Ca = function() {
        aj(this);
        for (var a = [], b = 0; b < this.A.length; b++) a.push(this.B[this.A[b]]);
        return a
    };
    q.kb = function() {
        aj(this);
        return this.A.concat()
    };
    q.Oa = function() {
        return 0 == this.ca
    };
    q.clear = function() {
        this.B = {};
        this.C = this.ca = this.A.length = 0
    };
    q.remove = function(a) {
        return bj(this.B, a) ? (delete this.B[a], this.ca--, this.C++, this.A.length > 2 * this.ca && aj(this), !0) : !1
    };

    function aj(a) {
        if (a.ca != a.A.length) {
            for (var b = 0, c = 0; b < a.A.length;) {
                var d = a.A[b];
                bj(a.B, d) && (a.A[c++] = d);
                b++
            }
            a.A.length = c
        }
        if (a.ca != a.A.length) {
            for (var e = {}, c = b = 0; b < a.A.length;) d = a.A[b], bj(e, d) || (a.A[c++] = d, e[d] = 1), b++;
            a.A.length = c
        }
    }
    q.get = function(a, b) {
        return bj(this.B, a) ? this.B[a] : b
    };
    q.set = function(a, b) {
        bj(this.B, a) || (this.ca++, this.A.push(a), this.C++);
        this.B[a] = b
    };
    q.forEach = function(a, b) {
        for (var c = this.kb(), d = 0; d < c.length; d++) {
            var e = c[d],
                f = this.get(e);
            a.call(b, f, e, this)
        }
    };
    q.qe = function(a) {
        aj(this);
        var b = 0,
            c = this.C,
            d = this,
            e = new Zi;
        e.next = function() {
            if (c != d.C) throw Error("The map has changed since the iterator was created");
            if (b >= d.A.length) throw Yi;
            var e = d.A[b++];
            return a ? e : d.B[e]
        };
        return e
    };

    function bj(a, b) {
        return Object.prototype.hasOwnProperty.call(a, b)
    };

    function cj(a) {
        if (a.Ca && "function" == typeof a.Ca) return a.Ca();
        if (wa(a)) return a.split("");
        if (va(a)) {
            for (var b = [], c = a.length, d = 0; d < c; d++) b.push(a[d]);
            return b
        }
        return Ob(a)
    }

    function dj(a) {
        if (a.kb && "function" == typeof a.kb) return a.kb();
        if (!a.Ca || "function" != typeof a.Ca) {
            if (va(a) || wa(a)) {
                var b = [];
                a = a.length;
                for (var c = 0; c < a; c++) b.push(c);
                return b
            }
            return Pb(a)
        }
    }

    function ej(a, b) {
        if (a.forEach && "function" == typeof a.forEach) a.forEach(b, void 0);
        else if (va(a) || wa(a)) nb(a, b, void 0);
        else
            for (var c = dj(a), d = cj(a), e = d.length, f = 0; f < e; f++) b.call(void 0, d[f], c && c[f], a)
    };

    function fj(a) {
        this.A = new $i;
        a && gj(this, a)
    }

    function hj(a) {
        var b = typeof a;
        return "object" == b && a || "function" == b ? "o" + Aa(a) : b.substr(0, 1) + a
    }
    q = fj.prototype;
    q.Bb = function() {
        return this.A.Bb()
    };
    q.add = function(a) {
        this.A.set(hj(a), a)
    };

    function gj(a, b) {
        b = cj(b);
        for (var c = b.length, d = 0; d < c; d++) a.add(b[d])
    }
    q.remove = function(a) {
        return this.A.remove(hj(a))
    };
    q.clear = function() {
        this.A.clear()
    };
    q.Oa = function() {
        return this.A.Oa()
    };
    q.contains = function(a) {
        a = hj(a);
        return bj(this.A.B, a)
    };

    function ij(a, b) {
        a = new fj(a);
        b = cj(b);
        for (var c = b.length, d = 0; d < c; d++) a.remove(b[d]);
        return a
    }
    q.Ca = function() {
        return this.A.Ca()
    };
    q.qe = function() {
        return this.A.qe(!1)
    };

    function jj(a) {
        var b;
        try {
            var c;
            var d = pa("window.location.href");
            if (wa(a)) c = {
                message: a,
                name: "Unknown error",
                lineNumber: "Not available",
                fileName: d,
                stack: "Not available"
            };
            else {
                var e, f, g = !1;
                try {
                    e = a.lineNumber || a.ik || "Not available"
                } catch (na) {
                    e = "Not available", g = !0
                }
                try {
                    f = a.fileName || a.filename || a.sourceURL || w.$googDebugFname || d
                } catch (na) {
                    f = "Not available", g = !0
                }
                c = !g && a.lineNumber && a.fileName && a.stack && a.message && a.name ? a : {
                    message: a.message || "Not available",
                    name: a.name || "UnknownError",
                    lineNumber: e,
                    fileName: f,
                    stack: a.stack || "Not available"
                }
            }
            var k;
            var l = c.fileName;
            null != l || (l = "");
            if (/^https?:\/\//i.test(l)) {
                var m = Gi(l),
                    n = ti("view-source scheme plus HTTP/HTTPS URL"),
                    p = "view-source:" + Ei(m);
                si(n);
                si(n);
                k = Hi(p)
            } else {
                var r = ti("sanitizedviewsrc");
                k = Hi(si(r))
            }
            var u = Ni("Message: " + c.message + "\nUrl: "),
                t;
            a = {
                href: k,
                target: "_new"
            };
            var v = c.fileName;
            if (!Oi.test("a")) throw Error("Invalid tag name <a>.");
            if ("A" in Qi) throw Error("Tag name <a> is not allowed for SafeHtml.");
            var d = null,
                x;
            e = "";
            if (a)
                for (var D in a) {
                    if (!Oi.test(D)) throw Error('Invalid attribute name "' +
                        D + '".');
                    var z = a[D];
                    if (null != z) {
                        f = e;
                        var A, g = D;
                        k = z;
                        if (k instanceof qi) k = si(k);
                        else if ("style" == g.toLowerCase()) {
                            m = l = void 0;
                            n = k;
                            if (!za(n)) throw Error('The "style" attribute requires goog.html.SafeStyle or map of style properties, ' + typeof n + " given: " + n);
                            if (!(n instanceof ui)) {
                                p = n;
                                r = "";
                                for (m in p) {
                                    if (!/^[-_a-zA-Z0-9]+$/.test(m)) throw Error("Name allows only [-_a-zA-Z0-9], got: " + m);
                                    var B = p[m];
                                    if (null != B) {
                                        if (B instanceof qi) B = si(B);
                                        else if (yi.test(B)) {
                                            for (var Q = !0, M = !0, G = 0; G < B.length; G++) {
                                                var T = B.charAt(G);
                                                "'" == T && M ? Q = !Q : '"' == T && Q && (M = !M)
                                            }
                                            Q && M || (B = "zClosurez")
                                        } else B = "zClosurez";
                                        r += m + ":" + B + ";"
                                    }
                                }
                                n = r ? wi(r) : xi
                            }
                            n instanceof ui && n.constructor === ui && n.B === vi ? l = n.A : (ta(n), l = "type_error:SafeStyle");
                            k = l
                        } else {
                            if (/^on/i.test(g)) throw Error('Attribute "' + g + '" requires goog.string.Const value, "' + k + '" given.');
                            if (g.toLowerCase() in Pi)
                                if (k instanceof zi) k = Bi(k);
                                else if (k instanceof Ci) k = Ei(k);
                            else if (wa(k)) k = Gi(k).lb();
                            else throw Error('Attribute "' + g + '" on tag "a" requires goog.html.SafeUrl, goog.string.Const, or string, value "' +
                                k + '" given.');
                        }
                        k.Yb && (k = k.lb());
                        A = g + '="' + Ma(String(k)) + '"';
                        e = f + (" " + A)
                    }
                }
            x = "<a" + e;
            null != v ? ua(v) || (v = [v]) : v = [];
            if (!0 === fi.a) x += ">";
            else {
                var R = Ri(v);
                x += ">" + Ki(R) + "</a>";
                d = R.uc()
            }
            var qa = a && a.dir;
            qa && (/^(ltr|rtl|auto)$/i.test(qa) ? d = 0 : d = null);
            t = Mi(x, d);
            b = Ri(u, t, Ni("\nLine: " + c.lineNumber + "\n\nBrowser stack:\n" + c.stack + "-> [end]\n\nJS stack traversal:\n" + kj(void 0) + "-> "))
        } catch (na) {
            b = Ni("Exception trying to expose exception! You win, we lose. " + na)
        }
        return Ki(b)
    }

    function kj(a) {
        var b;
        b = Error();
        if (Error.captureStackTrace) Error.captureStackTrace(b, a || kj), b = String(b.stack);
        else {
            try {
                throw b;
            } catch (c) {
                b = c
            }
            b = (b = b.stack) ? String(b) : null
        }
        b || (b = lj(a || arguments.callee.caller, []));
        return b
    }

    function lj(a, b) {
        var c = [];
        if (sb(b, a)) c.push("[...circular reference...]");
        else if (a && 50 > b.length) {
            c.push(mj(a) + "(");
            for (var d = a.arguments, e = 0; d && e < d.length; e++) {
                0 < e && c.push(", ");
                var f;
                f = d[e];
                switch (typeof f) {
                    case "object":
                        f = f ? "object" : "null";
                        break;
                    case "string":
                        break;
                    case "number":
                        f = String(f);
                        break;
                    case "boolean":
                        f = f ? "true" : "false";
                        break;
                    case "function":
                        f = (f = mj(f)) ? f : "[fn]";
                        break;
                    default:
                        f = typeof f
                }
                40 < f.length && (f = f.substr(0, 40) + "...");
                c.push(f)
            }
            b.push(a);
            c.push(")\n");
            try {
                c.push(lj(a.caller, b))
            } catch (g) {
                c.push("[exception trying to get caller]\n")
            }
        } else a ?
            c.push("[...long stack...]") : c.push("[end]");
        return c.join("")
    }

    function mj(a) {
        if (nj[a]) return nj[a];
        a = String(a);
        if (!nj[a]) {
            var b = /function ([^\(]+)/.exec(a);
            nj[a] = b ? b[1] : "[Anonymous]"
        }
        return nj[a]
    }
    var nj = {};
    var oj = {};

    function pj(a) {
        za(a) || (a = Error("" + a));
        return w.globals && w.globals.ErrorHandler ? w.globals.ErrorHandler.log(a, void 0) : a
    };

    function qj() {};

    function rj() {};
    var sj = [],
        tj = 1E3 / 30;

    function V(a, b) {
        this.D = a;
        this.H = b || [];
        this.F = [];
        this.G = !1
    }

    function uj(a, b) {
        vj[a] = b;
        if (b = wj[a]) {
            for (var c = 0, d = b.length; c < d; c++) b[c]();
            delete wj[a]
        }
    }

    function xj(a, b, c) {
        if (0 == a.length) b(c);
        else
            for (var d = a.length, e = function(a, c) {
                    --d || b(c)
                }, f = 0, g = a.length; f < g; f++) {
                var k = a[f];
                k ? k.C(e, c) : e(0, c)
            }
    }

    function yj(a, b, c) {
        var d;
        if (0 == a.length) b(c);
        else {
            var e = a.length,
                f = [],
                g = [],
                k = c.sa(b, "delayed:getMultiple"),
                l = function() {
                    --e || k(c)
                },
                m = function(a) {
                    return function() {
                        zj(a, c)
                    }
                };
            d = 0;
            for (b = a.length; d < b; d++) {
                var n = a[d];
                if (!n || n.B()) l();
                else {
                    n.F.push(l);
                    var p = n.D;
                    if (vj[p]) zj(n, c);
                    else {
                        f.push(n);
                        g.push(p);
                        var r = wj[p];
                        r || (r = wj[p] = []);
                        r.push(m(n))
                    }
                }
            }
        }
    }
    V.prototype.B = function() {
        return !!this.I
    };
    V.prototype.C = function(a, b) {
        Aj(this.D);
        if (this.B()) a(this.A(), b);
        else {
            var c = this;
            this.F.push(function(b) {
                a(c.A(), b)
            })
        }
    };
    V.prototype.get = function(a, b) {
        var c = this;
        yj([c], function(b) {
            a(c.A(), b)
        }, b)
    };
    V.prototype.A = h("I");
    var Bj = null,
        wj = {},
        vj = {},
        Aj = C;

    function zj(a, b) {
        Aj(a.D);
        try {
            if (!a.G) {
                var c = vj[a.D];
                a.G = !0;
                c.apply(null, vb(function(c) {
                    Aj(a.D);
                    a.I = c;
                    a.H = null;
                    c = "delayed:ready:" + a.D;
                    b.la(c);
                    try {
                        for (var d = 0, f = a.F.length; d < f; d++) a.F[d](b);
                        a.F = null
                    } finally {
                        b.done(c)
                    }
                }, b, a.H))
            }
        } catch (d) {
            throw qj(d.stack || jj(d)), pj(d);
        }
    };

    function Cj(a, b, c, d) {
        V.call(this, "CPNR", wb(arguments))
    }
    H(Cj, V);

    function Dj(a, b) {
        this.x = y(a) ? a : 0;
        this.y = y(b) ? b : 0
    }
    q = Dj.prototype;
    q.ceil = function() {
        this.x = Math.ceil(this.x);
        this.y = Math.ceil(this.y);
        return this
    };
    q.floor = function() {
        this.x = Math.floor(this.x);
        this.y = Math.floor(this.y);
        return this
    };
    q.round = function() {
        this.x = Math.round(this.x);
        this.y = Math.round(this.y);
        return this
    };
    q.translate = function(a, b) {
        a instanceof Dj ? (this.x += a.x, this.y += a.y) : (this.x += Number(a), xa(b) && (this.y += b));
        return this
    };
    q.scale = function(a, b) {
        b = xa(b) ? b : a;
        this.x *= a;
        this.y *= b;
        return this
    };

    function Ej(a, b) {
        this.width = a;
        this.height = b
    }
    q = Ej.prototype;
    q.ui = function() {
        return this.width * this.height
    };
    q.Oa = function() {
        return !this.ui()
    };
    q.ceil = function() {
        this.width = Math.ceil(this.width);
        this.height = Math.ceil(this.height);
        return this
    };
    q.floor = function() {
        this.width = Math.floor(this.width);
        this.height = Math.floor(this.height);
        return this
    };
    q.round = function() {
        this.width = Math.round(this.width);
        this.height = Math.round(this.height);
        return this
    };
    q.scale = function(a, b) {
        b = xa(b) ? b : a;
        this.width *= a;
        this.height *= b;
        return this
    };
    var Fj = !Rc || 9 <= Number(fd);
    !Tc && !Rc || Rc && 9 <= Number(fd) || Tc && ed("1.9.1");
    Rc && ed("9");

    function Gj(a, b) {
        a.innerHTML = Ki(b)
    };

    function Hj(a) {
        return a ? new Ij(Jj(a)) : Ia || (Ia = new Ij)
    }

    function Kj(a, b, c) {
        var d = document;
        c = c || d;
        a = a && "*" != a ? String(a).toUpperCase() : "";
        if (c.querySelectorAll && c.querySelector && (a || b)) return c.querySelectorAll(a + (b ? "." + b : ""));
        if (b && c.getElementsByClassName) {
            c = c.getElementsByClassName(b);
            if (a) {
                for (var d = {}, e = 0, f = 0, g; g = c[f]; f++) a == g.nodeName && (d[e++] = g);
                d.length = e;
                return d
            }
            return c
        }
        c = c.getElementsByTagName(a || "*");
        if (b) {
            d = {};
            for (f = e = 0; g = c[f]; f++) a = g.className, "function" == typeof a.split && sb(a.split(/\s+/), b) && (d[e++] = g);
            d.length = e;
            return d
        }
        return c
    }

    function Lj(a, b) {
        Mb(b, function(b, d) {
            "style" == d ? a.style.cssText = b : "class" == d ? a.className = b : "for" == d ? a.htmlFor = b : Mj.hasOwnProperty(d) ? a.setAttribute(Mj[d], b) : 0 == d.lastIndexOf("aria-", 0) || 0 == d.lastIndexOf("data-", 0) ? a.setAttribute(d, b) : a[d] = b
        })
    }
    var Mj = {
        cellpadding: "cellPadding",
        cellspacing: "cellSpacing",
        colspan: "colSpan",
        frameborder: "frameBorder",
        height: "height",
        maxlength: "maxLength",
        nonce: "nonce",
        role: "role",
        rowspan: "rowSpan",
        type: "type",
        usemap: "useMap",
        valign: "vAlign",
        width: "width"
    };

    function Nj(a, b, c) {
        return Oj(document, arguments)
    }

    function Oj(a, b) {
        var c = String(b[0]),
            d = b[1];
        if (!Fj && d && (d.name || d.type)) {
            c = ["<", c];
            d.name && c.push(' name="', Ma(d.name), '"');
            if (d.type) {
                c.push(' type="', Ma(d.type), '"');
                var e = {};
                Sb(e, d);
                delete e.type;
                d = e
            }
            c.push(">");
            c = c.join("")
        }
        c = a.createElement(c);
        d && (wa(d) ? c.className = d : ua(d) ? c.className = d.join(" ") : Lj(c, d));
        2 < b.length && Pj(a, c, b);
        return c
    }

    function Pj(a, b, c) {
        function d(c) {
            c && b.appendChild(wa(c) ? a.createTextNode(c) : c)
        }
        for (var e = 2; e < c.length; e++) {
            var f = c[e];
            !va(f) || za(f) && 0 < f.nodeType ? d(f) : nb(Qj(f) ? wb(f) : f, d)
        }
    }

    function Rj(a) {
        return document.createElement(String(a))
    }

    function Sj(a) {
        for (var b; b = a.firstChild;) a.removeChild(b)
    }

    function Tj(a, b) {
        b.parentNode && b.parentNode.insertBefore(a, b.nextSibling)
    }

    function Yj(a) {
        return a && a.parentNode ? a.parentNode.removeChild(a) : null
    }

    function Zj(a) {
        return y(a.firstElementChild) ? a.firstElementChild : ak(a.firstChild)
    }

    function bk(a) {
        return y(a.nextElementSibling) ? a.nextElementSibling : ak(a.nextSibling)
    }

    function ak(a) {
        for (; a && 1 != a.nodeType;) a = a.nextSibling;
        return a
    }

    function ck(a, b) {
        if (!a || !b) return !1;
        if (a.contains && 1 == b.nodeType) return a == b || a.contains(b);
        if ("undefined" != typeof a.compareDocumentPosition) return a == b || !!(a.compareDocumentPosition(b) & 16);
        for (; b && a != b;) b = b.parentNode;
        return b == a
    }

    function Jj(a) {
        return 9 == a.nodeType ? a : a.ownerDocument || a.document
    }

    function Qj(a) {
        if (a && "number" == typeof a.length) {
            if (za(a)) return "function" == typeof a.item || "string" == typeof a.item;
            if (ya(a)) return "function" == typeof a.item
        }
        return !1
    }

    function dk(a) {
        return window.matchMedia("(min-resolution: " + a + "dppx),(min--moz-device-pixel-ratio: " + a + "),(min-resolution: " + 96 * a + "dpi)").matches ? a : 0
    }

    function Ij(a) {
        this.A = a || w.document || document
    }
    q = Ij.prototype;
    q.Z = function(a) {
        return wa(a) ? this.A.getElementById(a) : a
    };
    q.getElementsByTagName = function(a, b) {
        return (b || this.A).getElementsByTagName(String(a))
    };
    q.He = function(a, b, c) {
        return Oj(this.A, arguments)
    };
    q.appendChild = function(a, b) {
        a.appendChild(b)
    };
    q.canHaveChildren = function(a) {
        if (1 != a.nodeType) return !1;
        switch (a.tagName) {
            case "APPLET":
            case "AREA":
            case "BASE":
            case "BR":
            case "COL":
            case "COMMAND":
            case "EMBED":
            case "FRAME":
            case "HR":
            case "IMG":
            case "INPUT":
            case "IFRAME":
            case "ISINDEX":
            case "KEYGEN":
            case "LINK":
            case "NOFRAMES":
            case "NOSCRIPT":
            case "META":
            case "OBJECT":
            case "PARAM":
            case "SCRIPT":
            case "SOURCE":
            case "STYLE":
            case "TRACK":
            case "WBR":
                return !1
        }
        return !0
    };
    q.removeNode = Yj;
    q.contains = ck;
    var ek = {
        Uh: 30,
        Vh: 4E-4,
        Th: .6,
        ze: function(a, b, c, d, e, f, g) {
            var k = b[0];
            b = b[1];
            var l = d[0];
            d = d[1];
            var m = f[0],
                n = f[1],
                p = d - n,
                r = n - b,
                u = b - d;
            f = k * p + l * r + m * u;
            if (0 == f) return !1;
            f = 1 / f;
            var p = p * f,
                t = (m - l) * f,
                v = (l * n - m * d) * f,
                r = r * f,
                x = (k - m) * f,
                m = (m * b - k * n) * f,
                u = u * f,
                n = (l - k) * f,
                k = (k * d - l * b) * f;
            g[0] = a[0] * p + c[0] * r + e[0] * u;
            g[1] = a[1] * p + c[1] * r + e[1] * u;
            g[2] = 0;
            g[3] = a[0] * t + c[0] * x + e[0] * n;
            g[4] = a[1] * t + c[1] * x + e[1] * n;
            g[5] = 0;
            g[6] = a[0] * v + c[0] * m + e[0] * k;
            g[7] = a[1] * v + c[1] * m + e[1] * k;
            g[8] = 1;
            return 1E-6 > Math.abs(g[0]) || 1E-6 > Math.abs(g[4]) ? !1 : !0
        },
        Gi: function(a,
            b, c, d) {
            var e = a.width,
                f = a.height,
                g = Nj("CANVAS", {
                    width: e,
                    height: f
                }),
                k = g.getContext("2d");
            b = b || ek.Uh;
            c = c || ek.Vh;
            d = d || ek.Th;
            for (var l = 0, m = 0; m < e; m += b) {
                var n = b;
                m + b > e && (n = e - m);
                var p = Math.max(f * (1 - m * c), 0),
                    r = Math.max(n * (1 - m * c), 0) * d;
                k.drawImage(a, m, 0, n, f, l, (f - p) / 2, r, p);
                l += r
            }
            return g
        }
    };

    function fk(a) {
        this.B = null;
        this.D = a;
        this.G = this.C = !1;
        this.A = new Float32Array(8)
    }
    fk.prototype.Ba = h("C");
    fk.prototype.Aa = h("B");
    fk.prototype.F = function() {
        if (!this.Ba() && !this.G) {
            var a = this.D.Aa();
            a ? (this.C = !0, this.B = ek.Gi(a), this.A[0] = this.A[1] = 0, this.A[2] = 0, this.A[3] = .5 * this.B.height, this.A[4] = .5 * this.B.width, this.A[5] = .5 * this.B.height, this.A[6] = .5 * this.B.width, this.A[7] = 0) : this.G = !0
        }
    };

    function gk(a) {
        this.length = a.length || a;
        for (var b = 0; b < this.length; b++) this[b] = a[b] || 0
    }
    gk.prototype.A = 4;
    gk.prototype.set = function(a, b) {
        b = b || 0;
        for (var c = 0; c < a.length && b + c < this.length; c++) this[b + c] = a[c]
    };
    gk.prototype.toString = Array.prototype.join;
    "undefined" == typeof Float32Array && (gk.BYTES_PER_ELEMENT = 4, gk.prototype.BYTES_PER_ELEMENT = gk.prototype.A, gk.prototype.set = gk.prototype.set, gk.prototype.toString = gk.prototype.toString, Ga("Float32Array", gk));

    function hk(a) {
        this.length = a.length || a;
        for (var b = 0; b < this.length; b++) this[b] = a[b] || 0
    }
    hk.prototype.A = 8;
    hk.prototype.set = function(a, b) {
        b = b || 0;
        for (var c = 0; c < a.length && b + c < this.length; c++) this[b + c] = a[c]
    };
    hk.prototype.toString = Array.prototype.join;
    if ("undefined" == typeof Float64Array) {
        try {
            hk.BYTES_PER_ELEMENT = 8
        } catch (a) {}
        hk.prototype.BYTES_PER_ELEMENT = hk.prototype.A;
        hk.prototype.set = hk.prototype.set;
        hk.prototype.toString = hk.prototype.toString;
        Ga("Float64Array", hk)
    };

    function ik() {
        return new Float64Array(9)
    }

    function jk(a, b) {
        var c = a[0],
            d = a[1],
            e = a[2],
            f = a[3],
            g = a[4],
            k = a[5],
            l = a[6],
            m = a[7];
        a = a[8];
        var n = g * a - m * k,
            p = m * e - d * a,
            r = d * k - g * e,
            u = c * n + f * p + l * r;
        0 != u && (u = 1 / u, b[0] = n * u, b[3] = (l * k - f * a) * u, b[6] = (f * m - l * g) * u, b[1] = p * u, b[4] = (c * a - l * e) * u, b[7] = (l * d - c * m) * u, b[2] = r * u, b[5] = (f * e - c * k) * u, b[8] = (c * g - f * d) * u)
    }

    function kk(a, b, c) {
        var d = b[0],
            e = b[1];
        b = b[2];
        c[0] = d * a[0] + e * a[3] + b * a[6];
        c[1] = d * a[1] + e * a[4] + b * a[7];
        c[2] = d * a[2] + e * a[5] + b * a[8]
    };

    function lk(a, b) {
        a[0] = b[0];
        a[1] = b[1];
        a[2] = b[2]
    };

    function mk() {
        return new Float32Array(4)
    }

    function nk(a, b, c, d, e) {
        a[0] = b;
        a[1] = c;
        a[2] = d;
        a[3] = e;
        return a
    };

    function ok() {
        return new Float32Array(16)
    }

    function pk(a, b) {
        a[0] = b[0];
        a[1] = b[1];
        a[2] = b[2];
        a[3] = b[3];
        a[4] = b[4];
        a[5] = b[5];
        a[6] = b[6];
        a[7] = b[7];
        a[8] = b[8];
        a[9] = b[9];
        a[10] = b[10];
        a[11] = b[11];
        a[12] = b[12];
        a[13] = b[13];
        a[14] = b[14];
        a[15] = b[15]
    }

    function qk(a, b, c) {
        var d = a[0],
            e = a[1],
            f = a[2],
            g = a[3],
            k = a[4],
            l = a[5],
            m = a[6],
            n = a[7],
            p = a[8],
            r = a[9],
            u = a[10],
            t = a[11],
            v = a[12],
            x = a[13],
            D = a[14];
        a = a[15];
        var z = b[0],
            A = b[1],
            B = b[2],
            Q = b[3],
            M = b[4],
            G = b[5],
            T = b[6],
            R = b[7],
            qa = b[8],
            na = b[9],
            sa = b[10],
            da = b[11],
            ea = b[12],
            gb = b[13],
            ib = b[14];
        b = b[15];
        c[0] = d * z + k * A + p * B + v * Q;
        c[1] = e * z + l * A + r * B + x * Q;
        c[2] = f * z + m * A + u * B + D * Q;
        c[3] = g * z + n * A + t * B + a * Q;
        c[4] = d * M + k * G + p * T + v * R;
        c[5] = e * M + l * G + r * T + x * R;
        c[6] = f * M + m * G + u * T + D * R;
        c[7] = g * M + n * G + t * T + a * R;
        c[8] = d * qa + k * na + p * sa + v * da;
        c[9] = e * qa + l * na + r * sa + x * da;
        c[10] = f * qa + m *
            na + u * sa + D * da;
        c[11] = g * qa + n * na + t * sa + a * da;
        c[12] = d * ea + k * gb + p * ib + v * b;
        c[13] = e * ea + l * gb + r * ib + x * b;
        c[14] = f * ea + m * gb + u * ib + D * b;
        c[15] = g * ea + n * gb + t * ib + a * b
    }

    function rk(a, b, c) {
        var d = b[0],
            e = b[1],
            f = b[2];
        b = b[3];
        c[0] = d * a[0] + e * a[4] + f * a[8] + b * a[12];
        c[1] = d * a[1] + e * a[5] + f * a[9] + b * a[13];
        c[2] = d * a[2] + e * a[6] + f * a[10] + b * a[14];
        c[3] = d * a[3] + e * a[7] + f * a[11] + b * a[15]
    }

    function sk(a, b) {
        var c = Math.cos(b),
            d = 1 - c;
        b = Math.sin(b);
        a[0] = 0 * d + c;
        a[1] = 0 * d + 1 * b;
        a[2] = 0 * d - 0 * b;
        a[3] = 0;
        a[4] = 0 * d - 1 * b;
        a[5] = 0 * d + c;
        a[6] = 0 * d + 0 * b;
        a[7] = 0;
        a[8] = 0 * d + 0 * b;
        a[9] = 0 * d - 0 * b;
        a[10] = 1 * d + c;
        a[11] = 0;
        a[12] = 0;
        a[13] = 0;
        a[14] = 0;
        a[15] = 1
    };

    function tk() {
        return new Float32Array(2)
    }

    function uk(a, b) {
        a[0] = b[0];
        a[1] = b[1]
    }

    function vk(a, b, c) {
        c[0] = a[0] + b[0];
        c[1] = a[1] + b[1]
    }

    function wk(a, b, c) {
        c[0] = a[0] - b[0];
        c[1] = a[1] - b[1]
    }

    function xk(a, b) {
        var c = a[0] - b[0];
        a = a[1] - b[1];
        return c * c + a * a
    };
    var yk = "closure_listenable_" + (1E6 * Math.random() | 0);

    function zk(a) {
        return !(!a || !a[yk])
    }
    var Ak = 0;

    function Bk() {
        this.M = this.M;
        this.K = this.K
    }
    Bk.prototype.M = !1;
    Bk.prototype.wa = h("M");
    Bk.prototype.ra = function() {
        this.M || (this.M = !0, this.ea())
    };

    function Ck(a, b) {
        a.M ? y(void 0) ? b.call(void 0) : b() : (a.K || (a.K = []), a.K.push(y(void 0) ? E(b, void 0) : b))
    }
    Bk.prototype.ea = function() {
        if (this.K)
            for (; this.K.length;) this.K.shift()()
    };

    function Dk(a) {
        a && "function" == typeof a.ra && a.ra()
    };

    function Ek(a, b) {
        this.type = a;
        this.currentTarget = this.target = b;
        this.defaultPrevented = this.ec = !1;
        this.oh = !0
    }
    Ek.prototype.stopPropagation = function() {
        this.ec = !0
    };
    Ek.prototype.preventDefault = function() {
        this.defaultPrevented = !0;
        this.oh = !1
    };
    var Fk = !Rc || 9 <= Number(fd),
        Gk = !Rc || 9 <= Number(fd),
        Hk = Rc && !ed("9");
    !Uc || ed("528");
    Tc && ed("1.9b") || Rc && ed("8") || Qc && ed("9.5") || Uc && ed("528");
    Tc && !ed("8") || Rc && ed("9");

    function Ik(a, b) {
        Ek.call(this, a ? a.type : "");
        this.relatedTarget = this.currentTarget = this.target = null;
        this.charCode = this.keyCode = this.button = this.screenY = this.screenX = this.clientY = this.clientX = this.offsetY = this.offsetX = 0;
        this.metaKey = this.shiftKey = this.altKey = this.ctrlKey = !1;
        this.ab = this.state = null;
        if (a) {
            var c = this.type = a.type,
                d = a.changedTouches ? a.changedTouches[0] : null;
            this.target = a.target || a.srcElement;
            this.currentTarget = b;
            if (b = a.relatedTarget) {
                if (Tc) {
                    var e;
                    a: {
                        try {
                            Tb(b.nodeName);
                            e = !0;
                            break a
                        } catch (f) {}
                        e = !1
                    }
                    e || (b = null)
                }
            } else "mouseover" == c ? b = a.fromElement : "mouseout" == c && (b = a.toElement);
            this.relatedTarget = b;
            null === d ? (this.offsetX = Uc || void 0 !== a.offsetX ? a.offsetX : a.layerX, this.offsetY = Uc || void 0 !== a.offsetY ? a.offsetY : a.layerY, this.clientX = void 0 !== a.clientX ? a.clientX : a.pageX, this.clientY = void 0 !== a.clientY ? a.clientY : a.pageY, this.screenX = a.screenX || 0, this.screenY = a.screenY || 0) : (this.clientX = void 0 !== d.clientX ? d.clientX : d.pageX, this.clientY = void 0 !== d.clientY ? d.clientY : d.pageY, this.screenX = d.screenX ||
                0, this.screenY = d.screenY || 0);
            this.button = a.button;
            this.keyCode = a.keyCode || 0;
            this.charCode = a.charCode || ("keypress" == c ? a.keyCode : 0);
            this.ctrlKey = a.ctrlKey;
            this.altKey = a.altKey;
            this.shiftKey = a.shiftKey;
            this.metaKey = a.metaKey;
            this.state = a.state;
            this.ab = a;
            a.defaultPrevented && this.preventDefault()
        }
    }
    H(Ik, Ek);
    var Jk = [1, 4, 2];

    function Kk(a, b) {
        return Fk ? a.ab.button == b : "click" == a.type ? 0 == b : !!(a.ab.button & Jk[b])
    }

    function Lk(a) {
        return Kk(a, 0) && !(Uc && Wc && a.ctrlKey)
    }
    Ik.prototype.stopPropagation = function() {
        Ik.V.stopPropagation.call(this);
        this.ab.stopPropagation ? this.ab.stopPropagation() : this.ab.cancelBubble = !0
    };
    Ik.prototype.preventDefault = function() {
        Ik.V.preventDefault.call(this);
        var a = this.ab;
        if (a.preventDefault) a.preventDefault();
        else if (a.returnValue = !1, Hk) try {
            if (a.ctrlKey || 112 <= a.keyCode && 123 >= a.keyCode) a.keyCode = -1
        } catch (b) {}
    };

    function Mk(a, b, c, d, e) {
        this.listener = a;
        this.A = null;
        this.src = b;
        this.type = c;
        this.capture = !!d;
        this.gb = e;
        this.key = ++Ak;
        this.zc = this.Kd = !1
    }

    function Nk(a) {
        a.zc = !0;
        a.listener = null;
        a.A = null;
        a.src = null;
        a.gb = null
    };

    function Ok(a) {
        this.src = a;
        this.A = {};
        this.B = 0
    }
    Ok.prototype.add = function(a, b, c, d, e) {
        var f = a.toString();
        a = this.A[f];
        a || (a = this.A[f] = [], this.B++);
        var g = Pk(a, b, d, e); - 1 < g ? (b = a[g], c || (b.Kd = !1)) : (b = new Mk(b, this.src, f, !!d, e), b.Kd = c, a.push(b));
        return b
    };
    Ok.prototype.remove = function(a, b, c, d) {
        a = a.toString();
        if (!(a in this.A)) return !1;
        var e = this.A[a];
        b = Pk(e, b, c, d);
        return -1 < b ? (Nk(e[b]), ub(e, b), 0 == e.length && (delete this.A[a], this.B--), !0) : !1
    };

    function Qk(a, b) {
        var c = b.type;
        if (!(c in a.A)) return !1;
        var d = tb(a.A[c], b);
        d && (Nk(b), 0 == a.A[c].length && (delete a.A[c], a.B--));
        return d
    }

    function Rk(a) {
        var b = 0,
            c;
        for (c in a.A) {
            for (var d = a.A[c], e = 0; e < d.length; e++) ++b, Nk(d[e]);
            delete a.A[c];
            a.B--
        }
    }
    Ok.prototype.Qc = function(a, b, c, d) {
        a = this.A[a.toString()];
        var e = -1;
        a && (e = Pk(a, b, c, d));
        return -1 < e ? a[e] : null
    };
    Ok.prototype.hasListener = function(a, b) {
        var c = y(a),
            d = c ? a.toString() : "",
            e = y(b);
        return Nb(this.A, function(a) {
            for (var f = 0; f < a.length; ++f)
                if (!(c && a[f].type != d || e && a[f].capture != b)) return !0;
            return !1
        })
    };

    function Pk(a, b, c, d) {
        for (var e = 0; e < a.length; ++e) {
            var f = a[e];
            if (!f.zc && f.listener == b && f.capture == !!c && f.gb == d) return e
        }
        return -1
    };
    var Sk = "closure_lm_" + (1E6 * Math.random() | 0),
        Tk = {},
        Uk = 0;

    function Vk(a, b, c, d, e) {
        if (ua(b)) {
            for (var f = 0; f < b.length; f++) Vk(a, b[f], c, d, e);
            return null
        }
        c = Wk(c);
        return zk(a) ? a.listen(b, c, d, e) : Xk(a, b, c, !1, d, e)
    }

    function Xk(a, b, c, d, e, f) {
        if (!b) throw Error("Invalid event type");
        var g = !!e,
            k = Yk(a);
        k || (a[Sk] = k = new Ok(a));
        c = k.add(b, c, d, e, f);
        if (c.A) return c;
        d = Zk();
        c.A = d;
        d.src = a;
        d.listener = c;
        if (a.addEventListener) a.addEventListener(b.toString(), d, g);
        else if (a.attachEvent) a.attachEvent($k(b.toString()), d);
        else throw Error("addEventListener and attachEvent are unavailable.");
        Uk++;
        return c
    }

    function Zk() {
        var a = al,
            b = Gk ? function(c) {
                return a.call(b.src, b.listener, c)
            } : function(c) {
                c = a.call(b.src, b.listener, c);
                if (!c) return c
            };
        return b
    }

    function bl(a, b, c, d, e) {
        if (ua(b)) {
            for (var f = 0; f < b.length; f++) bl(a, b[f], c, d, e);
            return null
        }
        c = Wk(c);
        return zk(a) ? a.Cg(b, c, d, e) : Xk(a, b, c, !0, d, e)
    }

    function cl(a, b, c, d, e) {
        if (ua(b))
            for (var f = 0; f < b.length; f++) cl(a, b[f], c, d, e);
        else c = Wk(c), zk(a) ? a.Ub(b, c, d, e) : a && (a = Yk(a)) && (b = a.Qc(b, c, !!d, e)) && dl(b)
    }

    function dl(a) {
        if (xa(a) || !a || a.zc) return !1;
        var b = a.src;
        if (zk(b)) return Qk(b.Za, a);
        var c = a.type,
            d = a.A;
        b.removeEventListener ? b.removeEventListener(c, d, a.capture) : b.detachEvent && b.detachEvent($k(c), d);
        Uk--;
        (c = Yk(b)) ? (Qk(c, a), 0 == c.B && (c.src = null, b[Sk] = null)) : Nk(a);
        return !0
    }

    function el(a) {
        if (a)
            if (zk(a)) a.Za && Rk(a.Za);
            else if (a = Yk(a)) {
            var b = 0,
                c;
            for (c in a.A)
                for (var d = a.A[c].concat(), e = 0; e < d.length; ++e) dl(d[e]) && ++b
        }
    }

    function $k(a) {
        return a in Tk ? Tk[a] : Tk[a] = "on" + a
    }

    function fl(a, b, c, d) {
        var e = !0;
        if (a = Yk(a))
            if (b = a.A[b.toString()])
                for (b = b.concat(), a = 0; a < b.length; a++) {
                    var f = b[a];
                    f && f.capture == c && !f.zc && (f = gl(f, d), e = e && !1 !== f)
                }
            return e
    }

    function gl(a, b) {
        var c = a.listener,
            d = a.gb || a.src;
        a.Kd && dl(a);
        return c.call(d, b)
    }

    function al(a, b) {
        if (a.zc) return !0;
        if (!Gk) {
            var c = b || pa("window.event");
            b = new Ik(c, this);
            var d = !0;
            if (!(0 > c.keyCode || void 0 != c.returnValue)) {
                a: {
                    var e = !1;
                    if (0 == c.keyCode) try {
                        c.keyCode = -1;
                        break a
                    } catch (g) {
                        e = !0
                    }
                    if (e || void 0 == c.returnValue) c.returnValue = !0
                }
                c = [];
                for (e = b.currentTarget; e; e = e.parentNode) c.push(e);a = a.type;
                for (e = c.length - 1; !b.ec && 0 <= e; e--) {
                    b.currentTarget = c[e];
                    var f = fl(c[e], a, !0, b),
                        d = d && f
                }
                for (e = 0; !b.ec && e < c.length; e++) b.currentTarget = c[e],
                f = fl(c[e], a, !1, b),
                d = d && f
            }
            return d
        }
        return gl(a, new Ik(b,
            this))
    }

    function Yk(a) {
        a = a[Sk];
        return a instanceof Ok ? a : null
    }
    var hl = "__closure_events_fn_" + (1E9 * Math.random() >>> 0);

    function Wk(a) {
        if (ya(a)) return a;
        a[hl] || (a[hl] = function(b) {
            return a.handleEvent(b)
        });
        return a[hl]
    };

    function il() {
        Bk.call(this);
        this.Za = new Ok(this);
        this.si = this;
        this.Qe = null
    }
    H(il, Bk);
    il.prototype[yk] = !0;
    q = il.prototype;
    q.Sd = h("Qe");
    q.bf = ba("Qe");
    q.addEventListener = function(a, b, c, d) {
        Vk(this, a, b, c, d)
    };
    q.removeEventListener = function(a, b, c, d) {
        cl(this, a, b, c, d)
    };
    q.dispatchEvent = function(a) {
        var b, c = this.Sd();
        if (c) {
            b = [];
            for (var d = 1; c; c = c.Sd()) b.push(c), ++d
        }
        c = this.si;
        d = a.type || a;
        if (wa(a)) a = new Ek(a, c);
        else if (a instanceof Ek) a.target = a.target || c;
        else {
            var e = a;
            a = new Ek(d, c);
            Sb(a, e)
        }
        var e = !0,
            f;
        if (b)
            for (var g = b.length - 1; !a.ec && 0 <= g; g--) f = a.currentTarget = b[g], e = jl(f, d, !0, a) && e;
        a.ec || (f = a.currentTarget = c, e = jl(f, d, !0, a) && e, a.ec || (e = jl(f, d, !1, a) && e));
        if (b)
            for (g = 0; !a.ec && g < b.length; g++) f = a.currentTarget = b[g], e = jl(f, d, !1, a) && e;
        return e
    };
    q.ea = function() {
        il.V.ea.call(this);
        this.Za && Rk(this.Za);
        this.Qe = null
    };
    q.listen = function(a, b, c, d) {
        return this.Za.add(String(a), b, !1, c, d)
    };
    q.Cg = function(a, b, c, d) {
        return this.Za.add(String(a), b, !0, c, d)
    };
    q.Ub = function(a, b, c, d) {
        return this.Za.remove(String(a), b, c, d)
    };

    function jl(a, b, c, d) {
        b = a.Za.A[String(b)];
        if (!b) return !0;
        b = b.concat();
        for (var e = !0, f = 0; f < b.length; ++f) {
            var g = b[f];
            if (g && !g.zc && g.capture == c) {
                var k = g.listener,
                    l = g.gb || g.src;
                g.Kd && Qk(a.Za, g);
                e = !1 !== k.call(l, d) && e
            }
        }
        return e && 0 != d.oh
    }
    q.Qc = function(a, b, c, d) {
        return this.Za.Qc(String(a), b, c, d)
    };
    q.hasListener = function(a, b) {
        return this.Za.hasListener(y(a) ? String(a) : void 0, b)
    };

    function kl(a, b, c, d) {
        this.handle = a;
        this.B = b;
        this.size = c;
        this.A = null;
        this.next = d
    }

    function ll() {
        this.G = this.D = this.F = 0;
        this.C = this.A = null;
        this.B = {}
    }
    ll.prototype.add = function(a, b) {
        if (a > this.F) return -1;
        var c = this.G++;
        b = new kl(c, b, a, this.A);
        this.B[c] = b;
        this.A && (this.A.A = b);
        this.A = b;
        this.D += a;
        null == this.C && (this.C = b);
        for (; this.D > this.F;) a = this.C, a.B && a.B(a.handle), this.remove(a.handle);
        return c
    };

    function ml(a, b) {
        (b = a.B[b]) && b.A && ((b.A.next = b.next) ? b.next.A = b.A : a.C = b.A, b.A = null, b.next = a.A, a.A.A = b, a.A = b)
    }
    ll.prototype.remove = function(a) {
        var b = this.B[a];
        b && (b.A ? b.A.next = b.next : this.A = b.next, b.next ? b.next.A = b.A : this.C = b.A, b.A = b.next = null, delete this.B[a], this.D -= b.size)
    };
    ll.prototype.contains = function(a) {
        return a in this.B
    };
    ll.prototype.clear = function() {
        for (var a = this.A; a; a = a.next) a.B && a.B(a.handle);
        this.C = this.A = null;
        this.B = {};
        this.D = 0
    };

    function nl(a, b) {
        this.B = new ll;
        this.B.F = a || Infinity;
        this.A = {};
        this.C = {};
        this.F = b || C
    }

    function ol(a, b, c) {
        var d = a.C[b];
        y(d) && -1 != d ? ml(a.B, d) : (d = a.B.add(1, E(a.D, a, b)), a.C[b] = d);
        a.A[b] = c
    }

    function pl(a, b) {
        var c = a.C[b];
        b = a.A[b];
        y(c) && -1 != c && ml(a.B, c);
        return b
    }
    nl.prototype.clear = function() {
        this.B.clear();
        this.A = {};
        this.C = {}
    };
    nl.prototype.D = function(a) {
        this.F(a, this.A[a]);
        delete this.C[a];
        delete this.A[a]
    };

    function ql() {
        this.height = this.width = this.L = this.N = this.D = this.F = this.J = this.G = this.C = this.B = this.A = this.M = this.K = this.I = this.H = void 0
    };

    function rl() {
        return new Float64Array(3)
    }

    function sl(a, b, c, d) {
        a[0] = b;
        a[1] = c;
        a[2] = d;
        return a
    }

    function tl(a, b) {
        a[0] = b[0];
        a[1] = b[1];
        a[2] = b[2]
    }

    function ul(a, b, c) {
        c[0] = a[0] + b[0];
        c[1] = a[1] + b[1];
        c[2] = a[2] + b[2]
    }

    function vl(a, b, c) {
        c[0] = a[0] - b[0];
        c[1] = a[1] - b[1];
        c[2] = a[2] - b[2]
    }

    function wl(a, b, c) {
        c[0] = a[0] * b;
        c[1] = a[1] * b;
        c[2] = a[2] * b
    }

    function xl(a) {
        var b = a[0],
            c = a[1];
        a = a[2];
        return Math.sqrt(b * b + c * c + a * a)
    }

    function yl(a, b) {
        var c = a[0],
            d = a[1];
        a = a[2];
        var e = 1 / Math.sqrt(c * c + d * d + a * a);
        b[0] = c * e;
        b[1] = d * e;
        b[2] = a * e
    }

    function zl(a, b) {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
    }

    function Al(a, b, c) {
        var d = a[0],
            e = a[1];
        a = a[2];
        var f = b[0],
            g = b[1];
        b = b[2];
        c[0] = e * b - a * g;
        c[1] = a * f - d * b;
        c[2] = d * g - e * f
    }

    function Bl(a, b) {
        var c = a[0] - b[0],
            d = a[1] - b[1];
        a = a[2] - b[2];
        return c * c + d * d + a * a
    };

    function Cl() {
        return new Float64Array(4)
    }

    function Dl(a, b, c) {
        c[0] = a[0] * b;
        c[1] = a[1] * b;
        c[2] = a[2] * b;
        c[3] = a[3] * b
    };

    function El() {
        return new Float64Array(16)
    }

    function Fl(a, b) {
        a[0] = b[0];
        a[1] = b[1];
        a[2] = b[2];
        a[3] = b[3];
        a[4] = b[4];
        a[5] = b[5];
        a[6] = b[6];
        a[7] = b[7];
        a[8] = b[8];
        a[9] = b[9];
        a[10] = b[10];
        a[11] = b[11];
        a[12] = b[12];
        a[13] = b[13];
        a[14] = b[14];
        a[15] = b[15]
    }

    function Gl(a, b) {
        a[0] = b[0];
        a[1] = b[1];
        a[2] = b[2];
        a[3] = b[3];
        a[4] = b[4];
        a[5] = b[5];
        a[6] = b[6];
        a[7] = b[7];
        a[8] = b[8];
        a[9] = b[9];
        a[10] = b[10];
        a[11] = b[11];
        a[12] = b[12];
        a[13] = b[13];
        a[14] = b[14];
        a[15] = b[15]
    }

    function Hl(a, b, c) {
        b *= 4;
        c[0] = a[b];
        c[1] = a[b + 1];
        c[2] = a[b + 2];
        c[3] = a[b + 3]
    }

    function Il(a, b, c) {
        a[b] = c[0];
        a[b + 4] = c[1];
        a[b + 8] = c[2];
        a[b + 12] = c[3]
    }

    function Jl(a, b) {
        var c = Kl;
        b[0] = c[a];
        b[1] = c[a + 4];
        b[2] = c[a + 8];
        b[3] = c[a + 12]
    }

    function Ll(a) {
        a[0] = 1;
        a[1] = 0;
        a[2] = 0;
        a[3] = 0;
        a[4] = 0;
        a[5] = 1;
        a[6] = 0;
        a[7] = 0;
        a[8] = 0;
        a[9] = 0;
        a[10] = 1;
        a[11] = 0;
        a[12] = 0;
        a[13] = 0;
        a[14] = 0;
        a[15] = 1
    }

    function Ml(a, b, c) {
        var d = a[0],
            e = a[1],
            f = a[2],
            g = a[3],
            k = a[4],
            l = a[5],
            m = a[6],
            n = a[7],
            p = a[8],
            r = a[9],
            u = a[10],
            t = a[11],
            v = a[12],
            x = a[13],
            D = a[14];
        a = a[15];
        var z = b[0],
            A = b[1],
            B = b[2],
            Q = b[3],
            M = b[4],
            G = b[5],
            T = b[6],
            R = b[7],
            qa = b[8],
            na = b[9],
            sa = b[10],
            da = b[11],
            ea = b[12],
            gb = b[13],
            ib = b[14];
        b = b[15];
        c[0] = d * z + k * A + p * B + v * Q;
        c[1] = e * z + l * A + r * B + x * Q;
        c[2] = f * z + m * A + u * B + D * Q;
        c[3] = g * z + n * A + t * B + a * Q;
        c[4] = d * M + k * G + p * T + v * R;
        c[5] = e * M + l * G + r * T + x * R;
        c[6] = f * M + m * G + u * T + D * R;
        c[7] = g * M + n * G + t * T + a * R;
        c[8] = d * qa + k * na + p * sa + v * da;
        c[9] = e * qa + l * na + r * sa + x * da;
        c[10] = f * qa + m *
            na + u * sa + D * da;
        c[11] = g * qa + n * na + t * sa + a * da;
        c[12] = d * ea + k * gb + p * ib + v * b;
        c[13] = e * ea + l * gb + r * ib + x * b;
        c[14] = f * ea + m * gb + u * ib + D * b;
        c[15] = g * ea + n * gb + t * ib + a * b
    }

    function Nl(a, b) {
        var c = a[0],
            d = a[1],
            e = a[2],
            f = a[3],
            g = a[4],
            k = a[5],
            l = a[6],
            m = a[7],
            n = a[8],
            p = a[9],
            r = a[10],
            u = a[11],
            t = a[12],
            v = a[13],
            x = a[14];
        a = a[15];
        var D = c * k - d * g,
            z = c * l - e * g,
            A = c * m - f * g,
            B = d * l - e * k,
            Q = d * m - f * k,
            M = e * m - f * l,
            G = n * v - p * t,
            T = n * x - r * t,
            R = n * a - u * t,
            qa = p * x - r * v,
            na = p * a - u * v,
            sa = r * a - u * x,
            da = D * sa - z * na + A * qa + B * R - Q * T + M * G;
        0 != da && (da = 1 / da, b[0] = (k * sa - l * na + m * qa) * da, b[1] = (-d * sa + e * na - f * qa) * da, b[2] = (v * M - x * Q + a * B) * da, b[3] = (-p * M + r * Q - u * B) * da, b[4] = (-g * sa + l * R - m * T) * da, b[5] = (c * sa - e * R + f * T) * da, b[6] = (-t * M + x * A - a * z) * da, b[7] = (n * M - r * A + u * z) * da, b[8] =
            (g * na - k * R + m * G) * da, b[9] = (-c * na + d * R - f * G) * da, b[10] = (t * Q - v * A + a * D) * da, b[11] = (-n * Q + p * A - u * D) * da, b[12] = (-g * qa + k * T - l * G) * da, b[13] = (c * qa - d * T + e * G) * da, b[14] = (-t * B + v * z - x * D) * da, b[15] = (n * B - p * z + r * D) * da)
    }

    function Ol(a, b, c) {
        var d = b[0],
            e = b[1];
        b = b[2];
        c[0] = d * a[0] + e * a[4] + b * a[8] + a[12];
        c[1] = d * a[1] + e * a[5] + b * a[9] + a[13];
        c[2] = d * a[2] + e * a[6] + b * a[10] + a[14]
    }

    function Pl(a, b, c) {
        var d = b[0],
            e = b[1];
        b = b[2];
        c[0] = d * a[0] + e * a[4] + b * a[8];
        c[1] = d * a[1] + e * a[5] + b * a[9];
        c[2] = d * a[2] + e * a[6] + b * a[10]
    }

    function Ql(a, b, c) {
        var d = b[0],
            e = b[1],
            f = b[2];
        b = b[3];
        c[0] = d * a[0] + e * a[4] + f * a[8] + b * a[12];
        c[1] = d * a[1] + e * a[5] + f * a[9] + b * a[13];
        c[2] = d * a[2] + e * a[6] + f * a[10] + b * a[14];
        c[3] = d * a[3] + e * a[7] + f * a[11] + b * a[15]
    }

    function Rl(a, b, c, d) {
        var e = Sl[0];
        vl(c, b, e);
        yl(e, e);
        e[3] = 0;
        c = Sl[1];
        Al(e, d, c);
        yl(c, c);
        c[3] = 0;
        d = Sl[2];
        Al(c, e, d);
        yl(d, d);
        d[3] = 0;
        e[0] = -e[0];
        e[1] = -e[1];
        e[2] = -e[2];
        Il(a, 0, c);
        Il(a, 1, d);
        Il(a, 2, e);
        a[3] = 0;
        a[7] = 0;
        a[11] = 0;
        a[15] = 1;
        Tl(a, -b[0], -b[1], -b[2])
    }

    function Tl(a, b, c, d) {
        a[12] += a[0] * b + a[4] * c + a[8] * d;
        a[13] += a[1] * b + a[5] * c + a[9] * d;
        a[14] += a[2] * b + a[6] * c + a[10] * d;
        a[15] += a[3] * b + a[7] * c + a[11] * d
    }

    function Ul(a, b, c, d) {
        a[0] *= b;
        a[1] *= b;
        a[2] *= b;
        a[3] *= b;
        a[4] *= c;
        a[5] *= c;
        a[6] *= c;
        a[7] *= c;
        a[8] *= d;
        a[9] *= d;
        a[10] *= d;
        a[11] *= d;
        a[12] = a[12];
        a[13] = a[13];
        a[14] = a[14];
        a[15] = a[15]
    }

    function Vl(a, b) {
        var c = a[4],
            d = a[5],
            e = a[6],
            f = a[7],
            g = a[8],
            k = a[9],
            l = a[10],
            m = a[11],
            n = Math.cos(b);
        b = Math.sin(b);
        a[4] = c * n + g * b;
        a[5] = d * n + k * b;
        a[6] = e * n + l * b;
        a[7] = f * n + m * b;
        a[8] = c * -b + g * n;
        a[9] = d * -b + k * n;
        a[10] = e * -b + l * n;
        a[11] = f * -b + m * n
    }

    function Wl(a, b) {
        var c = a[0],
            d = a[1],
            e = a[2],
            f = a[3],
            g = a[8],
            k = a[9],
            l = a[10],
            m = a[11],
            n = Math.cos(b);
        b = Math.sin(b);
        a[0] = c * n + g * -b;
        a[1] = d * n + k * -b;
        a[2] = e * n + l * -b;
        a[3] = f * n + m * -b;
        a[8] = c * b + g * n;
        a[9] = d * b + k * n;
        a[10] = e * b + l * n;
        a[11] = f * b + m * n
    }

    function Xl(a, b) {
        var c = a[0],
            d = a[1],
            e = a[2],
            f = a[3],
            g = a[4],
            k = a[5],
            l = a[6],
            m = a[7],
            n = Math.cos(b);
        b = Math.sin(b);
        a[0] = c * n + g * b;
        a[1] = d * n + k * b;
        a[2] = e * n + l * b;
        a[3] = f * n + m * b;
        a[4] = c * -b + g * n;
        a[5] = d * -b + k * n;
        a[6] = e * -b + l * n;
        a[7] = f * -b + m * n
    }
    var Sl = [Cl(), Cl(), Cl()];

    function Yl(a, b) {
        var c = 2 * Math.atan(Math.exp(a[1])) - Math.PI / 2;
        Zl(a[0], c, 6371010 * a[2] * Math.cos(c), b, 6371010)
    }

    function $l(a, b) {
        var c = a[0],
            d = a[1],
            e = a[2];
        a = Math.atan2(e, Math.sqrt(c * c + d * d));
        e = Math.sqrt(c * c + d * d + e * e) - 6371010;
        b[0] = Math.atan2(d, c);
        b[1] = a;
        b[2] = e;
        c = b[1];
        d = b[2];
        a = Math.sin(c);
        b[1] = .5 * Math.log((1 + a) / (1 - a));
        b[2] = d / (6371010 * Math.cos(c))
    }

    function am(a, b, c, d) {
        a = Jb(a);
        b = Jb(b);
        b = Eb(b, -1.48442222974533, 1.48442222974533);
        d[0] = a;
        a = Math.sin(b);
        d[1] = .5 * Math.log((1 + a) / (1 - a));
        d[2] = c / (6371010 * Math.cos(b))
    }

    function bm(a, b, c, d, e) {
        b = 2 * Math.atan(Math.exp(b)) - Math.PI / 2;
        c = c * (e || 6371010) * Math.cos(b);
        d[0] = a;
        d[1] = b;
        d[2] = c
    }

    function cm(a) {
        a = Jb(a);
        a = Eb(a, -1.48442222974533, 1.48442222974533);
        return 1 / (6371010 * Math.cos(a))
    }

    function dm(a, b, c, d, e) {
        Zl(Jb(a), Jb(b), c, d, e)
    }

    function Zl(a, b, c, d, e) {
        var f = Math.cos(b);
        c += e || 6371010;
        sl(d, c * f * Math.cos(a), c * f * Math.sin(a), c * Math.sin(b))
    }
    var em = rl(),
        fm = rl(),
        gm = rl(),
        hm = rl();
    var im = new ql,
        jm = rl();

    function km(a, b) {
        var c = se(a),
            d = ue(a),
            e = qe(a);
        a = pe(a);
        im.H = void 0;
        im.I = void 0;
        im.K = void 0;
        im.N = void 0;
        im.L = void 0;
        im.G = -Jb(Ae(c));
        im.J = Jb(N(c, 1));
        im.F = -Jb(N(c, 2));
        im.D = Jb(a);
        im.width = d.W();
        im.height = Ce(d);
        am(we(e), xe(e), ye(e), jm);
        im.A = jm[0];
        im.B = jm[1];
        im.C = jm[2];
        c = 1 * cm(xe(e));
        d = Math.abs(jm[2]);
        im.M = d > c ? d : c;
        lm(b, im);
        b.da = 0
    }

    function mm(a, b) {
        if (0 != a.da) throw Error("Invalid Coordinate System");
        nm(a, im);
        a = te(b);
        var c = ve(b),
            d = re(b);
        y(im.G) && (a.data[0] = Ib(-Kb(im.G)));
        y(im.J) && (a.data[1] = Ib(Kb(im.J)));
        y(im.F) && (a.data[2] = Ib(-Kb(im.F)));
        y(im.D) && (b.data[3] = Kb(im.D));
        y(im.width) && Be(c, im.width);
        y(im.height) && De(c, im.height);
        bm(im.A, im.B, im.C, jm, void 0);
        jm[0] = Kb(jm[0]);
        jm[1] = Kb(jm[1]);
        d.data[1] = jm[0];
        d.data[2] = jm[1];
        ze(d, jm[2])
    };

    function om(a, b) {
        il.call(this);
        this.S = a;
        this.D = [];
        this.G = [];
        this.H = new nl(b || 8)
    }
    H(om, il);
    q = om.prototype;
    q.xc = function(a) {
        km(a, this.S)
    };
    q.hc = function(a) {
        this.D = [];
        for (var b = 0; b < a.length; ++b) a[b] && this.D.push(a[b])
    };
    q.Pa = function() {
        this.$b();
        this.Md()
    };
    q.$b = function() {
        var a;
        for (a = 0; a < this.G.length; ++a) {
            var b = this.G[a],
                b = pm(this, b);
            b.F()
        }
        for (a = 0; a < this.D.length; ++a) b = this.D[a], b = pm(this, b), b.F()
    };
    q.Md = function() {
        for (var a = 0; a < this.D.length; ++a) {
            var b = pm(this, this.D[a]);
            this.B(b)
        }
    };

    function pm(a, b) {
        var c = pl(a.H, b.id());
        c || (c = a.I(b), ol(a.H, b.id(), c));
        return c
    }
    q.clear = function() {
        this.H.clear();
        this.D = [];
        this.G = []
    };
    q.yc = C;
    q.Zc = C;
    q.Yc = ca(!0);

    function qm(a, b) {
        om.call(this, a);
        this.A = b;
        this.C = b.canvas;
        this.F = null
    }
    H(qm, om);
    var rm = ik(),
        sm = tk(),
        tm = tk(),
        um = tk(),
        vm = mk(),
        wm = [mk(), mk(), mk(), mk()];
    qm.prototype.Md = function() {
        for (var a = 0; a < this.D.length; ++a) {
            var b = pm(this, this.D[a]);
            this.B(b)
        }
    };
    qm.prototype.I = function(a) {
        return new fk(a)
    };
    qm.prototype.B = function(a) {
        if (a.Ba() && this.F) {
            for (var b = a.D.A, c = xm(this.F), d = this.C.width, e = this.C.height, f = !1, g = 0; 4 > g; ++g) {
                nk(vm, b[3 * g], b[3 * g + 1], b[3 * g + 2], 1);
                var k = wm[g];
                rk(c, vm, k);
                var l = k[2] < -k[3] || k[1] < -k[3] || k[0] < -k[3] || k[2] > +k[3] || k[1] > +k[3] || k[0] > +k[3],
                    f = f || l;
                k[0] = (k[0] / k[3] + 1) * d / 2;
                k[1] = (-k[1] / k[3] + 1) * e / 2
            }
            f || (b = a.A, c = (b[2] + b[3]) / 2, sm[0] = (b[0] + b[1]) / 2, sm[1] = c, c = b[5], tm[0] = b[4], tm[1] = c, c = b[7], um[0] = b[6], um[1] = c, vk(wm[0], wm[1], vm), vm[0] *= .5, vm[1] *= .5, ek.ze(vm, sm, wm[2], tm, wm[3], um, rm) && (b = this.A,
                b.save(), b.setTransform(rm[0], rm[1], rm[3], rm[4], rm[6], rm[7]), b.drawImage(a.Aa(), 0, 0), b.restore()))
        }
    };

    function ym(a, b) {
        this.origin = new Float64Array(3);
        a && lk(this.origin, a);
        this.A = new Float64Array(3);
        b && lk(this.A, b)
    }
    ym.prototype.set = function(a, b) {
        lk(this.origin, a);
        lk(this.A, b)
    };

    function zm(a) {
        this.da = a || 0;
        this.X = ok();
        this.ha = ok();
        this.ja = ok();
        this.N = rl();
        this.na = Cl();
        sl(rl(), 1, 1, 1);
        this.L = El();
        this.$ = !0;
        this.G = this.F = this.D = 0;
        this.I = 1;
        this.U = this.P = this.O = this.C = this.B = this.A = 0;
        this.J = .4363323129985824;
        this.M = 1 / 3;
        this.K = Number.MAX_VALUE;
        this.Y = this.H = this.aa = 1;
        this.ga = []
    }
    zm.prototype.W = h("aa");

    function lm(a, b) {
        var c = !1,
            d = !1,
            e = !1,
            f = !1,
            g = !1;
        y(b.H) && (b.H != a.D && (g = !0, a.D = b.H), c = !0);
        y(b.I) && (b.I != a.F && (g = !0, a.F = b.I), c = !0);
        y(b.K) && (b.K != a.G && (g = !0, a.G = b.K), c = !0);
        y(b.M) && (b.M != a.I && (g = !0, a.I = b.M), d = !0);
        y(b.A) && (b.A != a.A && (g = !0, a.A = b.A), e = !0);
        y(b.B) && (b.B != a.B && (g = !0, a.B = b.B), e = !0);
        y(b.C) && (b.C != a.C && (g = !0, a.C = b.C), e = !0);
        y(b.G) && (b.G != a.O && (g = !0, a.O = b.G), f = !0);
        y(b.J) && (b.J != a.P && (g = !0, a.P = b.J), f = !0);
        y(b.F) && (b.F != a.U && (g = !0, a.U = b.F), f = !0);
        y(b.D) && b.D != a.J && (g = !0, a.J = b.D);
        y(b.N) && b.N != a.M && (g = !0,
            a.M = b.N);
        y(b.L) && b.L != a.K && (g = !0, a.K = b.L);
        y(b.width) && b.width != a.aa && (g = !0, a.aa = b.width);
        y(b.height) && b.height != a.H && (g = !0, a.H = b.height);
        if (g)
            for (!f || c || e || (e = !0), !d && e && c && (b = a.A - a.D, d = a.B - a.F, f = a.C - a.G, a.I = Math.sqrt(b * b + d * d + f * f)), e && !c && (Am(a, a.N), a.D = a.A + a.N[0], a.F = a.B + a.N[1], a.G = a.C + a.N[2]), c && !e && (Am(a, a.N), a.A = a.D - a.N[0], a.B = a.F - a.N[1], a.C = a.G - a.N[2]), a.$ = !0, a.Y++, c = 0; c < a.ga.length; c++) a.ga[c]()
    }

    function nm(a, b) {
        b = b || new ql;
        b.H = a.D;
        b.I = a.F;
        b.K = a.G;
        b.M = a.I;
        b.A = a.A;
        b.B = a.B;
        b.C = a.C;
        b.G = a.O;
        b.J = a.P;
        b.F = a.U;
        b.D = a.J;
        b.N = a.M;
        b.L = a.K;
        b.width = a.W();
        b.height = a.H;
        return b
    }

    function Bm(a) {
        Cm(a);
        return a.X
    }

    function Cm(a) {
        if (a.$) {
            var b = a.X,
                c = a.W() / a.H,
                d = a.M,
                e = a.K,
                f = a.J / 2,
                g = e - d,
                k = Math.sin(f);
            0 != g && 0 != k && 0 != c && (f = Math.cos(f) / k, b[0] = f / c, b[1] = 0, b[2] = 0, b[3] = 0, b[4] = 0, b[5] = f, b[6] = 0, b[7] = 0, b[8] = 0, b[9] = 0, b[10] = -(e + d) / g, b[11] = -1, b[12] = 0, b[13] = 0, b[14] = -(2 * d * e) / g, b[15] = 0);
            b = a.ja;
            e = -a.U;
            f = -a.P;
            d = -a.O;
            c = Math.cos(e);
            e = Math.sin(e);
            g = Math.cos(f);
            f = Math.sin(f);
            k = Math.cos(d);
            d = Math.sin(d);
            b[0] = c * k - g * e * d;
            b[1] = g * c * d + k * e;
            b[2] = d * f;
            b[3] = 0;
            b[4] = -c * d - k * g * e;
            b[5] = c * g * k - e * d;
            b[6] = k * f;
            b[7] = 0;
            b[8] = f * e;
            b[9] = -c * f;
            b[10] = g;
            b[11] = 0;
            b[12] =
                0;
            b[13] = 0;
            b[14] = 0;
            b[15] = 1;
            qk(a.X, a.ja, a.X);
            e = 1 / a.I;
            b = a.X;
            c = e * (a.D - a.A);
            d = e * (a.F - a.B);
            e *= a.G - a.C;
            b[12] += b[0] * c + b[4] * d + b[8] * e;
            b[13] += b[1] * c + b[5] * d + b[9] * e;
            b[14] += b[2] * c + b[6] * d + b[10] * e;
            b[15] += b[3] * c + b[7] * d + b[11] * e;
            var l = a.X,
                b = a.ha,
                c = l[0],
                d = l[1],
                e = l[2],
                g = l[3],
                f = l[4],
                k = l[5],
                m = l[6],
                n = l[7],
                p = l[8],
                r = l[9],
                u = l[10],
                t = l[11],
                v = l[12],
                x = l[13],
                D = l[14],
                l = l[15],
                z = c * k - d * f,
                A = c * m - e * f,
                B = c * n - g * f,
                Q = d * m - e * k,
                M = d * n - g * k,
                G = e * n - g * m,
                T = p * x - r * v,
                R = p * D - u * v,
                qa = p * l - t * v,
                na = r * D - u * x,
                sa = r * l - t * x,
                da = u * l - t * D,
                ea = z * da - A * sa + B * na + Q * qa - M * R + G * T;
            0 !=
                ea && (ea = 1 / ea, b[0] = (k * da - m * sa + n * na) * ea, b[1] = (-d * da + e * sa - g * na) * ea, b[2] = (x * G - D * M + l * Q) * ea, b[3] = (-r * G + u * M - t * Q) * ea, b[4] = (-f * da + m * qa - n * R) * ea, b[5] = (c * da - e * qa + g * R) * ea, b[6] = (-v * G + D * B - l * A) * ea, b[7] = (p * G - u * B + t * A) * ea, b[8] = (f * sa - k * qa + n * T) * ea, b[9] = (-c * sa + d * qa - g * T) * ea, b[10] = (v * M - x * B + l * z) * ea, b[11] = (-p * M + r * B - t * z) * ea, b[12] = (-f * na + k * R - m * T) * ea, b[13] = (c * na - d * R + e * T) * ea, b[14] = (-v * Q + x * A - D * z) * ea, b[15] = (p * Q - r * A + u * z) * ea);
            a.$ = !1
        }
    }

    function Dm(a, b, c, d) {
        d = d || new ym;
        var e = a.N,
            f = a.M,
            g = a.K;
        e[0] = b;
        e[1] = c;
        e[2] = g / (g - f);
        b = e || rl();
        c = e[1];
        var k = e[2];
        b[0] = 2 * e[0] / a.W() - 1;
        b[1] = 2 * -c / a.H + 1;
        b[2] = 2 * k - 1;
        e[2] = (g + f) / (g - f);
        f = a.L;
        Cm(a);
        Gl(f, a.ha);
        Ol(a.L, e, d.A);
        yl(d.A, d.A);
        sl(d.origin, a.A, a.B, a.C)
    }

    function Am(a, b) {
        sl(b, 0, 0, -a.I);
        var c = a.L,
            d = a.O,
            e = a.P,
            f = a.U,
            g = Math.cos(d),
            d = Math.sin(d),
            k = Math.cos(e),
            e = Math.sin(e),
            l = Math.cos(f),
            f = Math.sin(f);
        c[0] = g * l - k * d * f;
        c[1] = k * g * f + l * d;
        c[2] = f * e;
        c[3] = 0;
        c[4] = -g * f - l * k * d;
        c[5] = g * k * l - d * f;
        c[6] = l * e;
        c[7] = 0;
        c[8] = e * d;
        c[9] = -g * e;
        c[10] = k;
        c[11] = 0;
        c[12] = 0;
        c[13] = 0;
        c[14] = 0;
        c[15] = 1;
        Pl(a.L, b, b)
    };

    function Em(a, b) {
        this.A = a;
        this.B = b;
        this.M = this.B.getContext("2d");
        this.G = [];
        this.F = this.C = 0;
        this.D = new qm(new zm, this.A);
        this.K = "black"
    }
    var Fm = mk(),
        Gm = tk(),
        Hm = tk(),
        Im = tk(),
        Jm = tk(),
        Km = mk(),
        Lm = mk(),
        Mm = mk(),
        Nm = mk(),
        Om = null,
        Pm = ik(),
        Qm = ik();
    Em.prototype.H = function() {
        var a = this.A.canvas;
        this.A.clearRect(0, 0, a.width, a.height);
        this.A.fillStyle = this.K;
        this.A.fillRect(0, 0, a.width, a.height)
    };
    Em.prototype.I = function(a, b, c) {
        var d = N(b, 0);
        if (0 != d) {
            if (1 == d) Rm(this, c, this.A);
            else {
                if (this.A.canvas.width != this.B.width || this.A.canvas.height != this.B.height) this.B.width = this.A.canvas.width, this.B.height = this.A.canvas.height;
                this.M.clearRect(0, 0, this.B.width, this.B.height);
                Rm(this, c, this.M);
                this.A.globalAlpha = d;
                this.A.drawImage(this.B, 0, 0);
                this.A.globalAlpha = 1
            }
            b = N(b, 4);
            a = a.Nb();
            0 < b && 0 < a.length && (this.D.hc(a), this.D.F = c, this.D.Pa())
        }
    };

    function Rm(a, b, c) {
        var d = Sm(b);
        b = xm(b);
        for (var e = 0, f = d.length; e < f; ++e) {
            var g = a,
                k = d[e],
                l = b,
                m = c;
            if (k.Ba()) {
                var n = m.canvas.width,
                    p = m.canvas.height;
                Fm[0] = n / 2;
                Fm[1] = -p / 2;
                Fm[2] = n / 2;
                Fm[3] = p / 2;
                var n = k.Sc().Aa(),
                    p = Tm(k),
                    r = k.I,
                    u = Um(k),
                    t = u.length / 3,
                    k = Om;
                if (!k || k.length < 4 * t) k = Om = new Float32Array(4 * t);
                for (var v = 0; v < t; ++v) {
                    var x = Km,
                        D = Lm;
                    nk(x, u[3 * v], u[3 * v + 1], u[3 * v + 2], 1);
                    rk(l, x, D);
                    k[4 * v + 0] = D[0];
                    k[4 * v + 1] = D[1];
                    k[4 * v + 2] = D[2];
                    k[4 * v + 3] = D[3]
                }
                for (v = 0; v < p.length - 2; ++v)
                    if (g.C = 0, x = p[v], t = p[v + 1], D = p[v + 2], x != t && t != D && D != x) {
                        if (v <
                            p.length - 3 && (u = p[v + 3], t != u && u != D)) {
                            var l = g,
                                z = x,
                                A = t,
                                B = u,
                                Q = D,
                                t = k,
                                M = r,
                                D = n,
                                u = m;
                            Vm(t, z, Km);
                            Vm(t, A, Lm);
                            Vm(t, B, Mm);
                            Vm(t, Q, Nm);
                            var x = Wm(Km),
                                G = Wm(Lm),
                                T = Wm(Mm),
                                R = Wm(Nm);
                            if (!(x & G & T & R))
                                if ((x | G | T | R) & 1) Xm(l, z, A, B, t, M, D, u), Xm(l, z, B, Q, t, M, D, u);
                                else {
                                    Ym(Km);
                                    Ym(Lm);
                                    Ym(Mm);
                                    Ym(Nm);
                                    t = Zm(l);
                                    x = Zm(l);
                                    G = Zm(l);
                                    T = Zm(l);
                                    $m(M, z, t);
                                    $m(M, A, x);
                                    $m(M, B, G);
                                    $m(M, Q, T);
                                    M = Zm(l);
                                    R = Zm(l);
                                    A = Zm(l);
                                    z = Zm(l);
                                    wk(Lm, Km, M);
                                    wk(Mm, Lm, R);
                                    wk(Nm, Mm, A);
                                    wk(Km, Nm, z);
                                    Q = B = A;
                                    Q[0] = -1 * B[0];
                                    Q[1] = -1 * B[1];
                                    vk(M, A, M);
                                    B = A = z;
                                    B[0] = -1 * A[0];
                                    B[1] = -1 * A[1];
                                    vk(R, z, R);
                                    var A =
                                        Zm(l),
                                        B = Zm(l),
                                        Q = Zm(l),
                                        z = Zm(l),
                                        qa = A,
                                        na = B,
                                        sa = Q,
                                        da = z,
                                        ea = 1 / (M[0] * R[1] - M[1] * R[0]),
                                        gb = ea * R[1],
                                        ib = ea * -R[0],
                                        Hb = ea * -M[1],
                                        sc = ea * M[0],
                                        Ra = gb * Km[0] + ib * Km[1],
                                        ea = Hb * Km[0] + sc * Km[1],
                                        Ud = gb * Lm[0] + ib * Lm[1],
                                        Vi = Hb * Lm[0] + sc * Lm[1],
                                        Wi = gb * Mm[0] + ib * Mm[1],
                                        Xi = Hb * Mm[0] + sc * Mm[1],
                                        gb = gb * Nm[0] + ib * Nm[1],
                                        sc = Hb * Nm[0] + sc * Nm[1],
                                        Hb = an(Ra, Ud, Wi, gb),
                                        ib = an(ea, Vi, Xi, sc),
                                        Ra = bn(Ra, Ud, Wi, gb),
                                        ea = bn(ea, Vi, Xi, sc);
                                    qa[0] = M[0] * Hb + R[0] * ib;
                                    qa[1] = M[1] * Hb + R[1] * ib;
                                    na[0] = M[0] * Ra + R[0] * ib;
                                    na[1] = M[1] * Ra + R[1] * ib;
                                    sa[0] = M[0] * Ra + R[0] * ea;
                                    sa[1] = M[1] * Ra + R[1] * ea;
                                    da[0] =
                                        M[0] * Hb + R[0] * ea;
                                    da[1] = M[1] * Hb + R[1] * ea;
                                    M = bn(xk(Km, A), xk(Lm, B), xk(Mm, Q), xk(Nm, z));
                                    ek.ze(A, t, B, x, Q, G, Pm) && (A = Zm(l), A[0] = Pm[0] * T[0] + Pm[3] * T[1] + Pm[6], A[1] = Pm[1] * T[0] + Pm[4] * T[1] + Pm[7], z = xk(z, A), 4 < M || 4 < z ? (cn(l, Km, t, Lm, x, Nm, T, D, u), cn(l, Nm, T, Lm, x, Mm, G, D, u)) : (t = Zm(l), x = Zm(l), G = Zm(l), T = Zm(l), uk(t, Km), uk(x, Lm), uk(G, Mm), uk(T, Nm), z = x, A = G, B = T, Q = (t[0] + z[0] + A[0] + B[0]) / 4, M = (t[1] + z[1] + A[1] + B[1]) / 4, dn(t, Q, M), dn(z, Q, M), dn(A, Q, M), dn(B, Q, M), u.save(), u.beginPath(), u.moveTo(t[0], t[1]), u.lineTo(x[0], x[1]), u.lineTo(G[0], G[1]),
                                        u.lineTo(T[0], T[1]), u.closePath(), u.clip(), u.setTransform(Pm[0], Pm[1], Pm[3], Pm[4], Pm[6], Pm[7]), u.drawImage(D, 0, 0), u.restore(), ++l.F))
                                }++v;
                            continue
                        }
                        Xm(g, x, t, D, k, r, n, m)
                    }
            }
        }
        a.F = 0
    }

    function Zm(a) {
        a.C == a.G.length && (a.G[a.C] = tk());
        return a.G[a.C++]
    }

    function Vm(a, b, c) {
        nk(c, a[4 * b], a[4 * b + 1], a[4 * b + 2], a[4 * b + 3])
    }

    function $m(a, b, c) {
        var d = a[2 * b + 1];
        c[0] = a[2 * b];
        c[1] = d
    }

    function Wm(a) {
        return (a[2] < -a[3]) << 0 | (a[1] < -a[3]) << 1 | (a[0] < -a[3]) << 2 | (a[2] > +a[3]) << 3 | (a[1] > +a[3]) << 4 | (a[0] > +a[3]) << 5
    }

    function Ym(a) {
        a[0] = a[0] / a[3] * Fm[0] + Fm[2];
        a[1] = a[1] / a[3] * Fm[1] + Fm[3]
    }

    function bn(a, b, c, d) {
        a = b > a ? b : a;
        a = c > a ? c : a;
        return d > a ? d : a
    }

    function an(a, b, c, d) {
        a = b < a ? b : a;
        a = c < a ? c : a;
        return d < a ? d : a
    }

    function Xm(a, b, c, d, e, f, g, k) {
        var l = Km,
            m = Lm,
            n = Mm;
        Vm(e, b, l);
        Vm(e, c, m);
        Vm(e, d, n);
        e = Wm(l);
        var p = Wm(m),
            r = Wm(n);
        if (!(e & p & r)) {
            var u = Gm,
                t = Hm,
                v = Im;
            $m(f, b, u);
            $m(f, c, t);
            $m(f, d, v);
            f = (e & 1) + (p & 1) + (r & 1);
            if (1 == f) {
                for (; !(e & 1);) f = e, e = p, p = r, r = f, f = l, l = m, m = n, n = f, f = u, u = t, t = v, v = f, f = b, b = c, c = d, d = f;
                en(l, n, Nm, u, v, Jm);
                en(l, m, l, u, t, u);
                Ym(l);
                Ym(m);
                Ym(n);
                Ym(Nm);
                cn(a, l, u, m, t, Nm, Jm, g, k);
                cn(a, m, t, n, v, Nm, Jm, g, k)
            } else {
                if (2 == f) {
                    for (; e & 1;) f = e, e = p, p = r, r = f, f = l, l = m, m = n, n = f, f = u, u = t, t = v, v = f, f = b, b = c, c = d, d = f;
                    en(l, m, m, u, t, t);
                    en(l, n, n, u, v, v)
                }
                Ym(l);
                Ym(m);
                Ym(n);
                cn(a, l, u, m, t, n, v, g, k)
            }
        }
    }

    function cn(a, b, c, d, e, f, g, k, l) {
        ek.ze(b, c, d, e, f, g, Qm) && (c = Zm(a), e = Zm(a), g = Zm(a), uk(c, b), uk(e, d), uk(g, f), b = (c[0] + e[0] + g[0]) / 3, d = (c[1] + e[1] + g[1]) / 3, dn(c, b, d), dn(e, b, d), dn(g, b, d), l.save(), l.beginPath(), l.moveTo(c[0], c[1]), l.lineTo(e[0], e[1]), l.lineTo(g[0], g[1]), l.closePath(), l.clip(), l.setTransform(Qm[0], Qm[1], Qm[3], Qm[4], Qm[6], Qm[7]), l.drawImage(k, 0, 0), l.restore(), ++a.F)
    }

    function en(a, b, c, d, e, f) {
        var g = -a[2] - a[3],
            g = g / (g - (-b[2] - b[3])),
            k = a[0],
            l = a[1],
            m = a[2];
        a = a[3];
        c[0] = (b[0] - k) * g + k;
        c[1] = (b[1] - l) * g + l;
        c[2] = (b[2] - m) * g + m;
        c[3] = (b[3] - a) * g + a;
        b = d[0];
        d = d[1];
        f[0] = (e[0] - b) * g + b;
        f[1] = (e[1] - d) * g + d
    }

    function dn(a, b, c) {
        b = a[0] - b;
        c = a[1] - c;
        var d = Math.sqrt(b * b + c * c);
        1E-6 < d && (a[0] += 3 * b / d, a[1] += 3 * c / d)
    }
    Em.prototype.J = function(a) {
        this.K = 1 == a ? "white" : "black"
    };

    function fn(a, b, c, d) {
        this.C = a;
        this.D = b;
        this.B = c;
        this.I = d;
        this.K = a + "|" + b + "|" + c
    }
    fn.prototype.Ja = h("I");
    fn.prototype.toString = h("K");

    function gn(a, b, c, d, e) {
        fn.call(this, a, b, c, d);
        this.H = e
    }
    H(gn, fn);
    gn.prototype.Aa = ca(null);
    gn.prototype.Ba = ca(!0);
    gn.prototype.G = C;

    function hn(a, b, c, d, e) {
        gn.call(this, a, b, c, d, e);
        this.A = null
    }
    H(hn, gn);
    hn.prototype.Ba = function() {
        return !!this.A
    };
    hn.prototype.Aa = h("A");
    hn.prototype.G = function() {
        if (!this.Ba()) {
            var a;
            a = this.Ja();
            if (!Rc) {
                var b = a.width,
                    c = a.height,
                    d = Rj("canvas");
                d.width = b + 2;
                d.height = c + 2;
                var e = d.getContext("2d");
                e.drawImage(a, 0, 0, b, c, 1, 1, b, c);
                e.drawImage(a, 0, 0, b, 1, 1, 0, b, 1);
                e.drawImage(a, 0, c - 1, b, 1, 1, c + 1, b, 1);
                e.drawImage(d, 1, 0, 1, c + 2, 0, 0, 1, c + 1 + 2);
                e.drawImage(d, b, 0, 1, c + 2, b + 1, 0, 1, c + 1 + 2);
                a = d
            }
            this.A = a
        }
    };

    function jn(a) {
        this.length = a.length || a;
        for (var b = 0; b < this.length; b++) this[b] = a[b] || 0
    }
    jn.prototype.A = 2;
    jn.prototype.set = function(a, b) {
        b = b || 0;
        for (var c = 0; c < a.length && b + c < this.length; c++) this[b + c] = a[c]
    };
    jn.prototype.toString = function() {
        return Array.prototype.join(this)
    };
    "undefined" == typeof Uint16Array && (jn.BYTES_PER_ELEMENT = 2, jn.prototype.BYTES_PER_ELEMENT = jn.prototype.A, jn.prototype.set = jn.prototype.set, jn.prototype.toString = jn.prototype.toString, Ga("Uint16Array", jn));
    var kn = Cl(),
        ln = Cl();

    function mn(a) {
        a[0] = a[1] = a[2] = Infinity;
        a[3] = a[4] = a[5] = -Infinity
    }

    function nn(a, b) {
        for (var c = !0, d = 0; 6 > d; ++d) {
            var e;
            e = b[d];
            for (var f = e[3], g = e[3], k = 0; 3 > k; ++k) var l = 0 > e[k],
                m = l ? a[k] : a[3 + k],
                f = f + e[k] * (l ? a[3 + k] : a[k]),
                g = g + e[k] * m;
            e = 0 < f ? 1 : 0 < g ? 0 : -1;
            if (1 == e) return 0;
            0 == e && (c = !1)
        }
        return c ? 2 : 1
    };

    function on(a, b, c, d, e, f) {
        this.D = a;
        this.$ = b;
        this.G = c;
        this.H = d;
        this.L = e;
        this.J = 0;
        this.aa = f || !1;
        this.P = 1;
        this.C = this.B = this.U = this.F = this.K = this.X = null;
        this.Y = !0
    }
    var pn = new Float32Array(3);
    q = on.prototype;
    q.Sc = function() {
        var a = qn(this, !0);
        a && a.B != this.J && (this.X = a);
        return a
    };
    q.wg = function() {
        if (!this.Eb()) return !0;
        var a = qn(this);
        return !!a && !a.Ba()
    };
    q.Me = function() {
        this.ce();
        var a = qn(this);
        a && a.G()
    };
    q.Ne = function() {
        this.ce();
        var a = qn(this, !0);
        a || (a = qn(this));
        a && a.G()
    };
    q.Ba = function() {
        if (this.Eb()) {
            var a = this.Sc();
            return !!a && a.Ba()
        }
        return !1
    };
    q.Ag = function() {
        var a = rn(this, this.J);
        return !!a && a.Ba() && this.Eb()
    };
    q.Eb = ca(!0);
    q.ce = C;

    function qn(a, b) {
        for (var c = a.J; 0 <= c; --c) {
            var d = rn(a, c);
            if (d && (!b || d.Ba())) return d
        }
        for (var e = a.D.ma().A, e = Math.min(e, a.L), c = a.J + 1; c <= e; ++c)
            if ((d = rn(a, c)) && (!b || d.Ba())) return d;
        return null
    }

    function rn(a, b) {
        var c = Math.max(0, a.D.ma().A - b);
        (b = a.D.tb(a.G >> c, a.H >> c, b)) ? (c = a.$[b], c || (c = a.Ae(b, sn(a, b)), a.$[b] = c), a = c) : a = null;
        return a
    }

    function sn(a, b) {
        var c = a.D.ma();
        a = 1 << c.A - b.B;
        var d = b.C * a,
            e = b.D * a;
        b = tn(c, d);
        var f = tn(c, d + 1),
            g = un(c, e),
            k = f - b,
            f = un(c, e + 1) - g,
            l = c.C;
        d == Math.floor(l) && (k /= l - Math.floor(l));
        c = c.B;
        e == Math.floor(c) && (f /= c - Math.floor(c));
        c = 1 / k / a;
        a = 1 / f / a;
        return nk(mk(), c, a, -(b * c), -(g * a))
    }
    q.Ae = function(a, b) {
        return new gn(a.C, a.D, a.B, a.Ja(), b)
    };

    function vn(a) {
        wn(a);
        return a.K
    }

    function Um(a) {
        wn(a);
        return a.F
    }

    function xn(a, b, c, d) {
        var e = b[c],
            f = b[c + 1];
        b = b[c + 2];
        d && (d = 10 / Math.sqrt(e * e + f * f + b * b), e *= d, f *= d, b *= d);
        e < a[0] && (a[0] = e);
        f < a[1] && (a[1] = f);
        b < a[2] && (a[2] = b);
        e > a[3] && (a[3] = e);
        f > a[4] && (a[4] = f);
        b > a[5] && (a[5] = b)
    }

    function yn(a, b, c) {
        mn(b);
        var d = a.D.ma(),
            e = 1 << d.A - a.L,
            f = tn(d, a.G),
            g = tn(d, a.G + e),
            k = un(d, a.H);
        a = un(d, a.H + e);
        zn(d, f, k, pn, 0);
        xn(b, pn, 0, c);
        zn(d, g, k, pn, 0);
        xn(b, pn, 0, c);
        zn(d, f, a, pn, 0);
        xn(b, pn, 0, c);
        zn(d, g, a, pn, 0);
        xn(b, pn, 0, c);
        zn(d, (f + g) / 2, (k + a) / 2, pn, 0);
        xn(b, pn, 0, c)
    }

    function Tm(a) {
        wn(a);
        return a.U
    }

    function wn(a) {
        if (!(a.K && a.F && a.U && a.B))
            if (a.Y) {
                var b = a.D.ma();
                a.B || (a.B = new Float64Array(6));
                a.C || (a.C = new Float64Array(6));
                mn(a.B);
                mn(a.C);
                var c = 1 << b.A - a.L,
                    d = 7 * c + 1,
                    e = 7 * c + 1;
                a.K = new Float32Array(2 * d * e);
                a.F = new Float32Array(3 * d * e);
                for (var f = 0, g = tn(b, a.G), k = tn(b, a.G + c), l = un(b, a.H), m = un(b, a.H + c), c = 0; c < d; ++c)
                    for (var n = c / (d - 1), p = 0; p < e; ++p) {
                        var r = Gb(g, k, p / (e - 1)),
                            u = Gb(l, m, n);
                        a.K[2 * f] = r;
                        a.K[2 * f + 1] = u;
                        zn(b, r, u, a.F, 3 * f);
                        xn(a.B, a.F, 3 * f);
                        xn(a.C, a.F, 3 * f, !0);
                        ++f
                    }
                b = [];
                for (c = f = 0; c < d - 1; c++)
                    for (0 < c && (b[f++] = (c + 1) * e -
                            1, b[f++] = c * e), p = 0; p < e; p++) b[f++] = c * e + p, b[f++] = (c + 1) * e + p;
                a.U = new Uint16Array(b)
            } else An(a)
    }

    function An(a) {
        var b = a.D.ma(),
            c = a.G,
            d = a.H,
            e = a.aa,
            f = 1 << b.A - a.L,
            g = Math.min(c + f, b.N),
            k = Math.min(d + f, b.G),
            f = [],
            l = Math.PI * (2 * tn(b, c) - 1),
            m = Math.PI * (2 * tn(b, g) - 1),
            n = Math.PI * (.5 - un(b, d)),
            p = Math.PI * (.5 - un(b, k)),
            r = Math.cos(n) * (m - l),
            u = Math.cos(p) * (m - l),
            e = Jb(e ? 8 : 4),
            t = 1;
        1 == b.G && (t = Math.max(1, Math.ceil((m - l) / e)));
        f[0] = Math.max(1, Math.ceil((n - p) / e));
        f[1] = Math.max(t, Math.ceil(r / e));
        f[2] = Math.max(t, Math.ceil(u / e));
        f[0] = Bn(f[0], k - d);
        f[1] = Bn(f[1], g - c);
        f[2] = Bn(f[2], g - c);
        0 == d && (f[1] = f[2]);
        c = Math.round(f[0]) + 1;
        l = Math.round(f[1]) +
            1;
        m = Math.round(f[2]) + 1;
        f = [];
        for (d = k = 0; d < c; ++d)
            for (f[d] = [], n = Math.round(Gb(l, m, d / (c - 1))), g = 0; g < n; ++g) f[d][g] = k++;
        a.B || (a.B = new Float64Array(6));
        a.C || (a.C = new Float64Array(6));
        mn(a.B);
        mn(a.C);
        a.K = new Float32Array(2 * k);
        a.F = new Float32Array(3 * k);
        k = 0;
        d = 1 << b.A - a.L;
        l = tn(b, a.G);
        m = tn(b, a.G + d);
        n = un(b, a.H);
        p = un(b, a.H + d);
        for (d = 0; d < c; ++d)
            for (r = d / (c - 1), u = f[d].length, g = 0; g < u; ++g) e = Gb(l, m, g / (u - 1)), t = Gb(n, p, r), a.K[2 * k] = e, a.K[2 * k + 1] = t, zn(b, e, t, a.F, 3 * k), xn(a.B, a.F, 3 * k), xn(a.C, a.F, 3 * k, !0), ++k;
        b = [];
        for (d = k = 0; d < c - 1; d++)
            for (m =
                f[d].length, g = f[d + 1].length, 0 < d && (b[k++] = f[d][m - 1], b[k++] = f[d][0]), l = Math.max(m, g), m = (m - 1) / (l - 1), n = (g - 1) / (l - 1), g = 0; g < l; ++g) b[k++] = f[d][Math.round(g * m)], b[k++] = f[d + 1][Math.round(g * n)];
        a.U = new Uint16Array(b)
    };

    function Cn(a, b, c, d, e) {
        on.call(this, a, b, c, d, e, !0);
        this.M = !1;
        this.A = null;
        this.N = c + ":" + d;
        this.I = null
    }
    H(Cn, on);
    var Dn = mk();
    q = Cn.prototype;
    q.Eb = h("M");
    q.Ba = function() {
        return this.Eb() && !!this.A && this.A.Ba()
    };
    q.Sc = h("A");
    q.Ag = function() {
        var a = rn(this, this.J);
        return this.Eb() && !!a && a.Ba() && a === this.A
    };
    q.wg = function() {
        if (!this.Eb()) return !0;
        var a = qn(this);
        return !!a && !(a.Ba() && a === this.A)
    };
    q.Me = function() {
        Cn.V.Me.call(this);
        Ln(this)
    };
    q.Ne = function() {
        Cn.V.Ne.call(this);
        Ln(this)
    };
    q.ce = function() {
        this.Eb() || (this.M = !0)
    };
    q.toString = h("N");
    q.Ae = function(a, b) {
        return new hn(a.C, a.D, a.B, a.Ja(), b)
    };

    function Ln(a) {
        var b = qn(a, !0);
        if (b && a.A !== b) {
            a.A = b;
            var c = b.H,
                d = b.Aa(),
                b = d.width - 2,
                d = d.height - 2;
            Dn[0] = c[0] * b;
            Dn[1] = c[1] * d;
            Dn[2] = c[2] * b + 1;
            Dn[3] = c[3] * d + 1;
            0 != a.Y && (a.Y = !1, An(a));
            c = vn(a);
            b = c.length / 2;
            a.I || (a.I = new Float32Array(2 * b));
            a = a.I;
            for (var e = d = 0; d < b; ++d, e += 2) {
                var f = c[e + 1];
                a[e] = c[e] * Dn[0] + Dn[2];
                a[e + 1] = f * Dn[1] + Dn[3]
            }
        }
    }

    function Mn() {}
    Mn.prototype.create = function(a, b, c, d, e) {
        return new Cn(a, b, c, d, e)
    };

    function Nn() {
        return Uc ? "Webkit" : Tc ? "Moz" : Rc ? "ms" : Qc ? "O" : null
    }

    function On(a, b) {
        if (b && a in b) return a;
        var c = Nn();
        return c ? (c = c.toLowerCase(), a = c + jb(a), !y(b) || a in b ? a : null) : null
    };

    function Pn(a, b, c, d) {
        this.top = a;
        this.right = b;
        this.bottom = c;
        this.left = d
    }
    q = Pn.prototype;
    q.W = function() {
        return this.right - this.left
    };
    q.contains = function(a) {
        return this && a ? a instanceof Pn ? a.left >= this.left && a.right <= this.right && a.top >= this.top && a.bottom <= this.bottom : a.x >= this.left && a.x <= this.right && a.y >= this.top && a.y <= this.bottom : !1
    };
    q.ceil = function() {
        this.top = Math.ceil(this.top);
        this.right = Math.ceil(this.right);
        this.bottom = Math.ceil(this.bottom);
        this.left = Math.ceil(this.left);
        return this
    };
    q.floor = function() {
        this.top = Math.floor(this.top);
        this.right = Math.floor(this.right);
        this.bottom = Math.floor(this.bottom);
        this.left = Math.floor(this.left);
        return this
    };
    q.round = function() {
        this.top = Math.round(this.top);
        this.right = Math.round(this.right);
        this.bottom = Math.round(this.bottom);
        this.left = Math.round(this.left);
        return this
    };
    q.translate = function(a, b) {
        a instanceof Dj ? (this.left += a.x, this.right += a.x, this.top += a.y, this.bottom += a.y) : (this.left += a, this.right += a, xa(b) && (this.top += b, this.bottom += b));
        return this
    };
    q.scale = function(a, b) {
        b = xa(b) ? b : a;
        this.left *= a;
        this.right *= a;
        this.top *= b;
        this.bottom *= b;
        return this
    };

    function Qn(a, b, c, d) {
        this.left = a;
        this.top = b;
        this.width = c;
        this.height = d
    }
    q = Qn.prototype;
    q.contains = function(a) {
        return a instanceof Dj ? a.x >= this.left && a.x <= this.left + this.width && a.y >= this.top && a.y <= this.top + this.height : this.left <= a.left && this.left + this.width >= a.left + a.width && this.top <= a.top && this.top + this.height >= a.top + a.height
    };
    q.Pc = function() {
        return new Dj(this.left + this.width / 2, this.top + this.height / 2)
    };
    q.ceil = function() {
        this.left = Math.ceil(this.left);
        this.top = Math.ceil(this.top);
        this.width = Math.ceil(this.width);
        this.height = Math.ceil(this.height);
        return this
    };
    q.floor = function() {
        this.left = Math.floor(this.left);
        this.top = Math.floor(this.top);
        this.width = Math.floor(this.width);
        this.height = Math.floor(this.height);
        return this
    };
    q.round = function() {
        this.left = Math.round(this.left);
        this.top = Math.round(this.top);
        this.width = Math.round(this.width);
        this.height = Math.round(this.height);
        return this
    };
    q.translate = function(a, b) {
        a instanceof Dj ? (this.left += a.x, this.top += a.y) : (this.left += a, xa(b) && (this.top += b));
        return this
    };
    q.scale = function(a, b) {
        b = xa(b) ? b : a;
        this.left *= a;
        this.width *= a;
        this.top *= b;
        this.height *= b;
        return this
    };

    function Rn(a, b, c) {
        if (wa(b))(b = Sn(a, b)) && (a.style[b] = c);
        else
            for (var d in b) {
                c = a;
                var e = b[d],
                    f = Sn(c, d);
                f && (c.style[f] = e)
            }
    }
    var Tn = {};

    function Sn(a, b) {
        var c = Tn[b];
        if (!c) {
            var d = hb(b),
                c = d;
            void 0 === a.style[d] && (d = Nn() + jb(d), void 0 !== a.style[d] && (c = d));
            Tn[b] = c
        }
        return c
    }

    function Un(a, b) {
        var c;
        a: {
            c = Jj(a);
            if (c.defaultView && c.defaultView.getComputedStyle && (c = c.defaultView.getComputedStyle(a, null))) {
                c = c[b] || c.getPropertyValue(b) || "";
                break a
            }
            c = ""
        }
        return c || (a.currentStyle ? a.currentStyle[b] : null) || a.style && a.style[b]
    }

    function Vn(a) {
        var b;
        try {
            b = a.getBoundingClientRect()
        } catch (c) {
            return {
                left: 0,
                top: 0,
                right: 0,
                bottom: 0
            }
        }
        Rc && a.ownerDocument.body && (a = a.ownerDocument, b.left -= a.documentElement.clientLeft + a.body.clientLeft, b.top -= a.documentElement.clientTop + a.body.clientTop);
        return b
    }

    function Wn(a, b, c) {
        if (b instanceof Ej) c = b.height, b = b.width;
        else if (void 0 == c) throw Error("missing height argument");
        a.style.width = Xn(b);
        a.style.height = Xn(c)
    }

    function Xn(a) {
        "number" == typeof a && (a = Math.round(a) + "px");
        return a
    }

    function Yn(a) {
        var b = Zn;
        if ("none" != Un(a, "display")) return b(a);
        var c = a.style,
            d = c.display,
            e = c.visibility,
            f = c.position;
        c.visibility = "hidden";
        c.position = "absolute";
        c.display = "inline";
        a = b(a);
        c.display = d;
        c.position = f;
        c.visibility = e;
        return a
    }

    function Zn(a) {
        var b = a.offsetWidth,
            c = a.offsetHeight,
            d = Uc && !b && !c;
        return y(b) && !d || !a.getBoundingClientRect ? new Ej(b, c) : (a = Vn(a), new Ej(a.right - a.left, a.bottom - a.top))
    }

    function $n(a, b) {
        a = a.style;
        "opacity" in a ? a.opacity = b : "MozOpacity" in a ? a.MozOpacity = b : "filter" in a && (a.filter = "" === b ? "" : "alpha(opacity=" + 100 * Number(b) + ")")
    }

    function ao(a, b) {
        a.style.display = b ? "" : "none"
    };

    function bo(a, b) {
        return function(c) {
            c || (c = window.event);
            return b.call(a, c)
        }
    }

    function co(a) {
        a.preventDefault ? a.preventDefault() : a.returnValue = !1
    }

    function eo(a) {
        a = a.target || a.srcElement;
        !a.getAttribute && a.parentNode && (a = a.parentNode);
        return a
    }
    var fo = "undefined" != typeof navigator && /Macintosh/.test(navigator.userAgent),
        go = "undefined" != typeof navigator && !/Opera/.test(navigator.userAgent) && /WebKit/.test(navigator.userAgent),
        ho = "undefined" != typeof navigator && /WebKit/.test(navigator.userAgent) && /Safari/.test(navigator.userAgent),
        io = "undefined" != typeof navigator && (/MSIE/.test(navigator.userAgent) || /Trident/.test(navigator.userAgent));

    function jo() {
        this._mouseEventsPrevented = !0
    }

    function ko(a) {
        var b = w.document;
        if (b && !b.createEvent && b.createEventObject) try {
            return b.createEventObject(a)
        } catch (c) {
            return a
        } else return a
    };

    function lo(a, b, c, d, e) {
        il.call(this);
        this.J = a.replace(mo, "_");
        this.P = a;
        this.F = b || null;
        this.ab = c ? ko(c) : null;
        this.U = e || null;
        this.H = [];
        this.O = {};
        this.L = this.N = d || F();
        this.A = {};
        this.A["main-actionflow-branch"] = 1;
        this.D = new fj;
        this.B = !1;
        this.Vb = {};
        this.Nc = {};
        this.I = !1;
        c && b && "click" == c.type && this.action(b);
        no.push(this);
        this.ta = ++oo;
        a = new po("created", this);
        null != qo && qo.dispatchEvent(a)
    }
    H(lo, il);
    var no = [],
        qo = new il,
        mo = /[~.,?&-]/g,
        oo = 0;
    q = lo.prototype;
    q.id = h("ta");
    q.Fd = function() {
        this.I = !0
    };
    q.Vc = function(a) {
        this.J = a.replace(mo, "_");
        this.P = a
    };
    q.ua = function(a, b) {
        this.B && ro(this, "tick", void 0, a);
        b = b || {};
        a in this.O && this.D.add(a);
        var c = b.time || F();
        !b.Fi && !b.ln && c > this.L && (this.L = c);
        for (var d = c - this.N, e = this.H.length; 0 < e && this.H[e - 1][1] > d;) e--;
        yb(this.H, e, 0, [a, d, b.Fi]);
        this.O[a] = c
    };
    q.done = function(a, b, c) {
        this.B || !this.A[a] ? ro(this, "done", a, b) : (b && this.ua(b, c), this.A[a]--, 0 == this.A[a] && delete this.A[a], Qb(this.A) && so(this) && (this.B = !0, tb(no, this), this.ab = this.F = null, this.ra()))
    };
    q.la = function(a, b, c) {
        this.B && ro(this, "branch", a, b);
        b && this.ua(b, c);
        this.A[a] ? this.A[a]++ : this.A[a] = 1
    };

    function so(a) {
        if (!qo) return !0;
        if (a.I) {
            var b = new po("abandoned", a);
            a.dispatchEvent(b);
            qo.dispatchEvent(b);
            return !0
        }
        0 < a.D.Bb() && (a.Nc.dup = a.D.Ca().join("|"));
        b = new po("beforedone", a);
        if (!a.dispatchEvent(b) || !qo.dispatchEvent(b)) return !1;
        var c = to(a.Nc);
        c && (a.Vb.cad = c);
        b.type = "done";
        return qo.dispatchEvent(b)
    }

    function ro(a, b, c, d) {
        if (qo) {
            var e = new po("error", a);
            e.error = b;
            e.la = c;
            e.B = d;
            e.A = a.B;
            qo.dispatchEvent(e)
        }
    }

    function to(a) {
        var b = [];
        Mb(a, function(a, d) {
            d = encodeURIComponent(d);
            encodeURIComponent(a).replace(/%7C/g, "|");
            b.push(d + ":" + a)
        });
        return b.join(",")
    }
    q.action = function(a) {
        this.B && ro(this, "action");
        var b = [],
            c = null,
            d = null,
            e = null,
            f = null;
        uo(a, function(a) {
            var g;
            !a.__oi && a.getAttribute && (a.__oi = a.getAttribute("oi"));
            if (g = a.__oi) b.unshift(g), c || (c = a.getAttribute("jsinstance"));
            e || d && "1" != d || (e = a.getAttribute("ved"));
            f || (f = a.getAttribute("vet"));
            d || (d = a.getAttribute("jstrack"))
        });
        f && (this.Vb.vet = f);
        d && (this.Vb.ct = this.J, 0 < b.length && vo(this, "oi", b.join(".")), c && (c = "*" == c.charAt(0) ? parseInt(c.substr(1), 10) : parseInt(c, 10), this.Vb.cd = c), "1" != d && (this.Vb.ei =
            d), e && (this.Vb.ved = e))
    };

    function vo(a, b, c) {
        a.B && ro(a, "extradata");
        a.Nc[b] = c.toString().replace(/[:;,\s]/g, "_")
    }

    function uo(a, b) {
        for (; a && 1 == a.nodeType; a = a.parentNode) b(a)
    }
    q.sa = function(a, b, c, d) {
        this.la(b, c);
        var e = this;
        return function() {
            try {
                var c = a.apply(this, arguments)
            } finally {
                e.done(b, d)
            }
            return c
        }
    };
    q.node = h("F");
    q.event = h("ab");
    q.rb = h("U");
    q.value = function(a) {
        var b = this.F;
        return b ? a in b ? b[a] : b.getAttribute ? b.getAttribute(a) : void 0 : void 0
    };

    function po(a, b) {
        Ek.call(this, a, b);
        this.oa = b
    }
    H(po, Ek);

    function wo(a, b, c, d, e) {
        lo.call(this, b, c, d, e);
        this.G = a;
        this.C = null
    }
    H(wo, lo);
    q = wo.prototype;
    q.ob = function(a, b) {
        this.C = b;
        vo(this, "an", b);
        this.G.start(a, b, this)
    };
    q.Dg = function(a, b) {
        this.C = b;
        vo(this, "an", b);
        return xo(this.G, this, a, b)
    };
    q.Wc = function() {
        return !!yo(this.G, this)
    };
    q.ua = function(a, b) {
        wo.V.ua.call(this, a, b)
    };
    q.Fd = function() {
        wo.V.Fd.call(this)
    };
    q.Vc = function(a) {
        wo.V.Vc.call(this, a)
    };

    function zo() {
        wo.call(this, {}, "NULL_FLOW");
        this.Fd();
        wo.prototype.done.call(this, "main-actionflow-branch")
    }
    H(zo, wo);
    q = zo.prototype;
    q.la = aa();
    q.done = aa();
    q.ob = aa();
    q.Dg = ca(!1);
    q.Wc = ca(!1);

    function Ao(a, b, c) {
        Bk.call(this);
        this.B = a;
        this.Wb = b;
        this.D = c;
        this.C = [];
        this.A = Bo++
    }
    H(Ao, Bk);
    var Bo = 1;
    Ao.prototype.cancel = function() {
        if (!this.wa()) {
            for (var a = 0; a < this.C.length; a++) this.C[a](this.D);
            this.ra()
        }
    };
    Ao.prototype.ea = function() {
        this.D = null;
        this.C.length = 0
    };

    function Co(a, b, c) {
        Mb(a.J(), function(a, e) {
            vo(b, c + e, "" + a)
        })
    };

    function Do(a) {
        this.C = a;
        this.B = new $i;
        this.A = {};
        this.D = {};
        this.G = {};
        this.F = {};
        this.H = {};
        Mb(this.C, function(a, c) {
            this.A[c] = {};
            this.D[c] = 0
        }, this)
    }

    function Eo(a, b, c) {
        var d = b.prefix;
        if (b = b.mh.B()) a = b.L(a), Co(a, c.oa, d || "")
    }

    function Fo(a, b, c) {
        var d = a.B.get(b.id());
        a.B.remove(b.id());
        d && (delete a.A[d.B][d.Wb], c && d.cancel(), d.ra())
    }

    function yo(a, b) {
        return Qb(b.A) ? (Fo(a, b, !1), null) : a.B.get(b.id()) || null
    }

    function Go(a, b, c) {
        return (b = a.A[b] && a.A[b][c]) ? yo(a, b) : null
    }

    function Ho(a, b, c, d) {
        var e = a.A[b] && a.A[b][c];
        e && (Qb(e.A) || (e.ua("int"), vo(e, "ian", d)), Go(a, b, c), Fo(a, e, !0))
    }

    function Io(a, b, c, d) {
        var e = a.A[b];
        if (!(a.D[b] > c)) {
            for (var f in e)(e = Go(a, b, f)) && e.A < c && Ho(a, b, f, d);
            a.D[b] = c
        }
    }

    function Jo(a, b, c) {
        return (a = a.C[b]) && !!a.actions[c]
    }

    function xo(a, b, c, d) {
        if (!Jo(a, c, d)) return !1;
        var e = yo(a, b);
        if (!e) return !1;
        if (e.B == c && e.Wb == d) return !0;
        if (a.D[c] > e.A) return !1;
        var f = a.C[c];
        if (f.Kb) {
            var g = Go(a, c, d);
            if (g && g.A > e.A) return !1
        }
        for (g = 0; g < f.Ab.length; g++) Io(a, f.Ab[g], e.A, d);
        Ho(a, c, d, d);
        e.B = c;
        e.Wb = d;
        a.A[e.B][e.Wb] = b;
        a.B.set(b.id(), e);
        f.Kb || Io(a, c, e.A, d);
        return !0
    }
    Do.prototype.start = function(a, b, c) {
        if (!Jo(this, a, b) || yo(this, c)) return null;
        for (var d = new Ao(a, b, c), e = this.C[a], f = 0; f < e.Ab.length; f++) Io(this, e.Ab[f], d.A, b);
        e.Kb ? Ho(this, a, b, b) : Io(this, a, d.A, b);
        a = e.actions[b].ka;
        for (f = 0; f < a.length; f++) {
            if (e = this.G[a[f]])
                for (b = 0; b < e.length; b++) new Ko(e[b], a[f], c);
            if (e = this.F[a[f]])
                for (b = 0; b < e.length; b++) {
                    var g = e[b].mh.B();
                    g && Vk(c, "beforedone", Fa(Eo, g, e[b]))
                }
            if (e = this.H[a[f]])
                for (b = 0; b < e.length; b++) e[b].en(c)
        }
        this.A[d.B][d.Wb] = c;
        this.B.set(c.id(), d);
        return d
    };

    function Lo(a, b, c) {
        c = {
            prefix: void 0,
            mh: c
        };
        a.F[b] || (a.F[b] = []);
        a.F[b].push(c)
    }

    function Ko(a, b, c) {
        this.B = a;
        this.C = "actionmanager.flowgate-" + b;
        this.A = !1;
        this.F = Vk(c, "beforedone", E(this.D, this))
    }
    Ko.prototype.D = function(a) {
        var b = a.oa;
        if (!this.A && this.B.An(b)) {
            this.A = !0;
            b.la(this.C);
            var c = this;
            this.B.fn(function() {
                c.A = !1;
                b.done(c.C);
                Qb(b.A) && dl(c.F)
            }, b)
        }
        return !this.A
    };

    function Mo() {
        var a = {};
        (a.init = {
            Kb: !0,
            Ab: [],
            actions: {}
        }).actions.application_init = {
            ka: ["render"]
        };
        var b = a.card = {
            Kb: !0,
            Ab: [],
            actions: {}
        };
        b.actions.star = {
            ka: ["render"]
        };
        b.actions.unstar = {
            ka: ["render"]
        };
        b = a.scene = {
            Kb: !0,
            Ab: ["transitions"],
            actions: {}
        };
        b.actions.click_scene = {
            ka: ["render"]
        };
        b.actions.move_camera = {
            ka: ["render", "camera_change"]
        };
        b.actions.scroll_zoom = {
            ka: ["render", "camera_change"]
        };
        b = a.scene_hover = {
            Kb: !0,
            Ab: [],
            actions: {}
        };
        b.actions.hover_on_map = {
            ka: []
        };
        b.actions.hover_on_poi = {
            ka: ["render"]
        };
        b = a.transitions = {
            Kb: !1,
            Ab: ["scene"],
            actions: {}
        };
        b.actions.clear_map = {
            ka: ["render"]
        };
        b.actions.compose_directions_request = {
            ka: ["render"]
        };
        b.actions.directions_drag = {
            ka: ["render"]
        };
        b.actions.directions_inspect_step = {
            ka: ["render"]
        };
        b.actions.directions_inspect_step_done = {
            ka: ["render"]
        };
        b.actions.get_directions = {
            ka: ["render"]
        };
        b.actions.high_confidence_suggest = {
            ka: ["render"]
        };
        b.actions.highlight_suggestion = {
            ka: ["render"]
        };
        b.actions.manual_url_change = {
            ka: ["render"]
        };
        b.actions.search = {
            ka: ["render", "camera_change"]
        };
        b.actions.spotlight_alternate_route = {
            ka: ["render"]
        };
        b.actions.spotlight_implicit_route = {
            ka: ["render"]
        };
        b.actions.spotlight_indoor = {
            ka: ["render"]
        };
        b.actions.spotlight_poi = {
            ka: ["render"]
        };
        b.actions.spotlight_reveal = {
            ka: ["render"]
        };
        b.actions.spotlight_suggestion = {
            ka: ["render"]
        };
        b.actions.suggest = {
            ka: ["render"]
        };
        b.actions.switch_map_mode = {
            ka: ["render"]
        };
        b.actions.switch_to_map_mode = {
            ka: ["render"]
        };
        b.actions.switch_to_text_mode = {
            ka: ["render"]
        };
        b = a.runway = {
            Kb: !1,
            Ab: [],
            actions: {}
        };
        b.actions.change_runway_state = {
            ka: []
        };
        b.actions.toggle_lookbook = {
            ka: []
        };
        return a
    };
    var No = new il;
    new il;
    var Oo = null;

    function Po() {
        Oo || (Oo = new Do(Mo()));
        return Oo
    }

    function Qo(a, b, c) {
        a = Ro(a);
        return Vk(a, b, So(c), !1)
    }

    function To(a, b, c, d) {
        var e;
        d instanceof Ek ? (e = d, e.type = b) : e = new Ek(b);
        e.nk = {
            event: d,
            oa: c
        };
        Ro(a).dispatchEvent(e)
    }

    function Uo(a, b, c) {
        a = Ro(a);
        var d = Ro(c);
        return Vk(a, b, function(a) {
            d.dispatchEvent(a)
        })
    }

    function Ro(a) {
        if (a.dispatchEvent) return a.Sd || (a.Sd = C), a;
        a.kf = a.kf || new il;
        return a.kf
    }

    function So(a) {
        return function(b) {
            var c = b.nk;
            c ? a.call(void 0, c.oa, c.event) : b instanceof po ? a.call(void 0, new zo, b) : (c = new wo(Oo, "event_" + b.type), a.call(void 0, c, b), c.done("main-actionflow-branch"))
        }
    };

    function Vo(a) {
        this.data = a || []
    }
    H(Vo, J);

    function Wo(a) {
        return new le(a.data[2])
    };
    var Xo = rl(),
        Yo = new zm;

    function Zo(a) {
        return a ? 2 === a.sb() || 3 === a.sb() : !1
    }

    function $o(a) {
        return a ? 4 === a.sb() : !1
    }

    function ap(a, b) {
        if (a.length !== b.length) return !1;
        for (var c = 0; c < a.length; ++c)
            if (a[c] !== b[c]) return !1;
        return !0
    }

    function bp(a, b) {
        var c = cp;
        a = a || [];
        b = b || [];
        for (var d = a.length > b.length ? a.length : b.length, e = 0; e < d; ++e)
            if ((a[e] || c) !== (b[e] || c)) return !1;
        return !0
    }

    function dp(a, b) {
        km(a, Yo);
        Xo[0] = b[0] - Yo.A;
        Xo[1] = b[1] - Yo.B;
        Xo[2] = b[2] - Yo.C;
        b = Kb(Fb(Math.atan2(Xo[0], Xo[1]), 2 * Math.PI));
        var c = Kb(Math.atan2(Xo[2], Math.sqrt(Xo[0] * Xo[0] + Xo[1] * Xo[1])) + Math.PI / 2);
        te(a).data[0] = b;
        te(a).data[1] = c
    };

    function ep(a, b, c, d) {
        il.call(this);
        this.D = function() {
            fp(a)
        };
        this.F = a.A;
        this.S = new zm;
        this.C = !1;
        this.H = d || 6;
        this.L = new nl(this.H);
        this.N = c;
        this.J = b;
        this.A = [];
        this.B = [];
        this.G = new fj;
        this.I = []
    }
    H(ep, il);
    q = ep.prototype;
    q.hc = function(a, b, c) {
        if (!ap(this.A, a) || !bp(this.B, c)) {
            this.C = !1;
            gp(this);
            gj(this.G, this.A);
            this.G = ij(this.G, a);
            hp(this);
            this.A = [];
            this.B = [];
            for (var d = a.length, d = d > this.H ? this.H : d, e = 0; e < d; ++e) {
                var f = a[e];
                if (!$o(f)) {
                    this.A.push(f);
                    var g = cp;
                    c && c[e] && (g = c[e]);
                    this.B.push(g);
                    if (Zo(f)) ip(this, f, g, this.D, b);
                    else {
                        f.Rb(b);
                        var k = this;
                        f.dc(b.sa(function(a) {
                            4 != a && 0 != a && ip(k, f, g, k.D, b)
                        }, "br-onready"))
                    }
                }
            }
            jp(this)
        }
    };

    function ip(a, b, c, d, e) {
        Zo(b) && (Ic(c, 5) || kp(lp(a, b, c), e));
        b = lp(a, b, c);
        mp(b, a.F, e, a.D);
        np(b, a.F, e, d)
    }
    q.Pa = function() {
        this.C = !1;
        this.J.H();
        for (var a = 0, b = this.A.length; a < b; ++a) {
            var c = this.A[a],
                d = this.B[a];
            if (Zo(c) && 0 !== N(d, 0)) {
                var e = lp(this, c, d),
                    f;
                f = e;
                if (f.H) {
                    for (var g = (F() - f.H) / 400, k = [], l = 0, m = f.I.length; l < m; l++) {
                        var n = f.I[l];
                        n.P = g;
                        1 > g && k.push(n)
                    }
                    f.I = k;
                    f.I.length || (f.H = 0);
                    f = !!f.I.length
                } else f = !1;
                f && this.D();
                this.J.I(c, d, e)
            }
        }
        this.C = !1;
        if (0 != this.A.length) {
            a = !0;
            for (b = 0; b < this.A.length; ++b) {
                c = lp(this, this.A[b], this.B[b]);
                a: if (d = c, 0 == d.B.length) d = !1;
                    else {
                        for (e = 0; e < d.B.length; ++e)
                            if (!d.B[e].Ag()) {
                                d = !1;
                                break a
                            }
                        d = !0
                    }
                if (!d || c.H) {
                    a = !1;
                    break
                }
            }
            this.C = a
        }
        this.C && this.dispatchEvent(new Ek("ViewportReady", this))
    };
    q.$b = function(a) {
        op(this, a);
        pp(this, a)
    };

    function qp(a, b, c) {
        var d = rp,
            e;
        a: {
            for (e = a.A.length - 1; 0 <= e; e--) {
                var f = a.B[e];
                if (K(f, 0) && 1 == N(f, 0)) break a
            }
            e = -1
        }
        if (f = -1 != e)
            if (f = a.A[e], Zo(f)) {
                var g = a.B[e];
                Dm(a.S, b, c, sp);
                a = lp(a, f, g);
                b = sp;
                tp(a);
                up(a.A.ma(), b, a.da, d);
                f = 0 <= d.x && 1 >= d.x && 0 <= d.y && 1 >= d.y ? !0 : !1
            } else f = !1;
        return f ? e : -1
    }

    function hp(a) {
        for (var b = a.G.Ca(); 0 < b.length;) {
            var c = b.shift();
            c.Kc();
            var d = c = lp(a, c, cp);
            d.F && (d.F.cancel(a.F), d.F = null);
            c.K && (c.K.cancel(a.F), c.K = null)
        }
        a.G.clear()
    }

    function pp(a, b) {
        for (var c = 0; c < a.A.length; ++c) {
            var d = a,
                e = b,
                f = a.D,
                g = lp(d, a.A[c], a.B[c]);
            mp(g, d.F, e, d.D);
            np(g, d.F, e, f)
        }
    }

    function op(a, b) {
        for (var c = 0; c < a.A.length; ++c) {
            var d = a.A[c],
                e = a.B[c],
                f = b;
            Zo(d) && (Ic(e, 5) || kp(lp(a, d, e), f))
        }
    }

    function lp(a, b, c) {
        var d = pl(a.L, b.id());
        d || (d = a.N.create(b, c, a.S), ol(a.L, b.id(), d));
        d.G = c;
        return d
    }
    q.fa = h("S");

    function jp(a) {
        for (var b = 0; b < a.A.length; ++b) {
            var c = Qo(a.A[b], "TileReady", function(b) {
                pp(a, b)
            });
            a.I.push(c)
        }
    }

    function gp(a) {
        for (var b = 0; b < a.A.length; ++b) dl(a.I[b]);
        a.I = []
    }
    q.clear = function() {
        gp(this);
        gj(this.G, this.A);
        hp(this);
        this.L.clear();
        this.A = [];
        this.B = []
    };
    q.yc = C;
    q.Yc = h("C");
    q.Zc = function(a) {
        this.J.J(a)
    };
    var vp = new Vo;
    vp.data[0] = 1;
    vp.data[4] = 1;
    vp.data[1] = 0;
    var cp = vp,
        sp = new ym;
    ep.prototype.xc = function(a, b) {
        this.C = !1;
        km(a, this.S);
        a = cm(xe(qe(a)));
        var c = nm(this.S);
        c.M = .01 * a;
        c.H = void 0;
        c.I = void 0;
        c.K = void 0;
        lm(this.S, c);
        op(this, b);
        pp(this, b)
    };

    function wp(a, b) {
        this.A = rl();
        this.B = new Float64Array(2);
        this.D = a;
        this.C = a / (2 * Math.PI);
        this.G = 1 / this.C;
        this.F = b / Math.PI;
        this.H = 1 / this.F
    }

    function xp(a, b) {
        var c = Math.acos(b[2]) * a.F - .5;
        b = (Math.atan2(b[0], b[1]) + Math.PI) * a.C;
        b >= a.D - .5 && (b -= a.D);
        a.B[0] = b;
        a.B[1] = c;
        return a.B
    }

    function yp(a, b, c) {
        var d = (c + .5) * a.H;
        c = Math.sin(d);
        d = Math.cos(d);
        b = 1.5 * Math.PI - b * a.G;
        var e = Math.sin(b);
        a.A[0] = c * Math.cos(b);
        a.A[1] = c * e;
        a.A[2] = d;
        return a.A
    };
    new ym;

    function zp(a, b, c, d, e, f, g) {
        this.C = a;
        this.B = b;
        this.F = f || 0;
        this.G = g || 0;
        this.H = d || null;
        this.I = e || 0;
        this.A = c || [];
        this.D = new wp(a, b)
    }
    var Ap = sl(rl(), 0, 0, 1),
        Bp = rl(),
        Cp = rl(),
        Dp = Cl(),
        Ep = new zp(512, 512, null, null, 0, 500),
        Fp = new zp(512, 512, null, null, 0, 0, 3);
    zp.prototype.W = h("C");

    function Gp(a, b, c, d) {
        if (0 == a.C || 0 == a.B || !a.D) return null;
        b = Eb(b, 0, 1);
        c = Eb(c, 0, 1);
        b *= a.W() - 1;
        c = yp(a.D, b, c * (a.B - 1));
        d = d || new ym;
        a = Hp(a, c, d.A);
        if (0 == a) return null;
        wl(c, a, d.origin);
        return d
    }

    function Ip(a, b, c) {
        c && tl(c, Ap);
        return 0 == a.F && 0 < a.G && 0 > b[2] ? -a.G * xl(b) / b[2] : a.F
    }

    function Hp(a, b, c) {
        if (!a.A || !a.A.length) return Ip(a, b, c);
        var d = xp(a.D, b),
            e = Jp(a, d[0], d[1], void 0);
        if (0 >= e) return Ip(a, b, c);
        var e = 4 * e,
            d = a.A[e++],
            f = a.A[e++],
            g = a.A[e++];
        a = a.A[e++];
        c && (c[0] = d, c[1] = f, c[2] = g);
        return Eb(a / (b[0] * d + b[1] * f + b[2] * g), .1, 500)
    }

    function Jp(a, b, c, d) {
        if (!a.H) return 0;
        b = Math.floor(b + .5);
        c = Math.floor(c + .5);
        b >= a.C ? b -= a.C : 0 > b && (b += a.C);
        c >= a.B ? c -= a.B : 0 > c && (c += a.B);
        d && (d[0] = b, d[1] = c);
        return Kp(a.H, a.I + c * a.C + b) || 0
    };

    function Lp(a, b, c, d) {
        ep.call(this, a, b, c, d)
    }
    H(Lp, ep);
    var Mp = new ym,
        rp = new Dj;
    Lp.prototype.Rc = function(a, b, c) {
        var d = qp(this, a, b);
        if (-1 == d) return null;
        var e = this.A[d],
            d = this.B[d];
        Dm(this.S, a, b, Mp);
        a = e.ma();
        Np(a, Op, K(d, 2) ? Wo(d) : void 0);
        Nl(Op, Pp);
        up(0, Mp, Pp, Qp);
        (c = Gp(a.F, Qp.x, Qp.y, c)) ? (Ol(Op, c.origin, c.origin), Pl(Op, c.A, c.A), yl(c.A, c.A)) : c = null;
        return c
    };
    Lp.prototype.Tc = ca(null);
    Lp.prototype.Yd = function(a) {
        a[0] = 1;
        a[1] = 179
    };
    Lp.prototype.Rd = function(a, b, c) {
        a = qp(this, a, b);
        return -1 == a ? null : this.A[a].Qd(rp, c)
    };

    function Rp(a, b, c, d, e) {
        this.D = a;
        this.F = b;
        this.C = c;
        this.G = d;
        (a = e) || (a = Float64Array, d.B || (d.B = new Float64Array(6), yn(d, d.B, !1)), a = new a(d.B));
        this.B = a;
        this.A = []
    }

    function Sp(a, b, c, d, e) {
        if (!(3 <= a.C || a.C >= d)) {
            a.A = [];
            Tp(a, b, c, d, e);
            for (var f = 0; f < a.A.length; f++) Sp(a.A[f], b, c, d, e);
            for (b = 0; b < a.A.length; b++) {
                c = a.A[b].B;
                for (d = 0; 3 > d; d++) a.B[d] = Math.min(a.B[d], c[d]);
                for (d = 3; 6 > d; d++) a.B[d] = Math.max(a.B[d], c[d])
            }
        }
    }

    function Up(a, b, c, d, e, f, g, k) {
        if (!nn(a.B, b)) return [];
        var l = k(a.D, a.F),
            l = Math.min(l, f),
            m = a.G;
        m.J = Math.min(l, m.L);
        if (a.C >= l) return c && (c = a.G, c.C || (c.C = new Float64Array(6), yn(c, c.C, !0)), c = !nn(c.C, b)), c ? [] : [a.G];
        0 == a.A.length && Tp(a, d, e, f, g);
        l = [];
        for (m = 0; m < a.A.length; m++) l = l.concat(Up(a.A[m], b, c, d, e, f, g, k));
        return l
    }

    function Tp(a, b, c, d, e) {
        var f = a.C + 1;
        if (!(f > d)) {
            var g = 1 << d - f;
            d = a.D;
            var k = a.D + g,
                l = a.F,
                g = a.F + g;
            Vp(a, d, l, f, b, c, e);
            Vp(a, k, l, f, b, c, e);
            Vp(a, d, g, f, b, c, e);
            Vp(a, k, g, f, b, c, e)
        }
    }

    function Vp(a, b, c, d, e, f, g) {
        b >= e || c >= f || !(e = g(b, c, d)) || (b = new Rp(b, c, d, e), a.A.push(b))
    };

    function Wp(a, b, c) {
        this.G = b;
        this.A = !0;
        b.la("img-patch-prepare");
        this.B = a;
        this.F = c || C
    }
    Wp.prototype.start = function() {
        return this.A ? 0 == this.B.length ? (Xp(this), rj) : this.D() : rj
    };

    function Xp(a) {
        a.F();
        a.G.done("img-patch-prepare");
        a.A = !1
    }
    Wp.prototype.D = function() {
        var a = this.B.shift();
        this.C(a);
        return 0 == this.B.length ? (Xp(this), rj) : this.D
    };
    Wp.prototype.cancel = function() {
        this.A && (this.__maps_realtime_JobScheduler_next_step = null, Xp(this))
    };
    Wp.prototype.C = function(a) {
        a.Me()
    };

    function Yp(a, b, c) {
        Wp.call(this, a, b, c)
    }
    H(Yp, Wp);
    Yp.prototype.C = function(a) {
        a.Ne()
    };

    function Zp(a, b, c, d, e) {
        Ek.call(this, a, b);
        this.x = c;
        this.y = d;
        this.zoom = e
    }
    H(Zp, Ek);

    function $p() {
        il.call(this);
        this.A = null;
        this.B = [];
        this.C = "" + Aa(this)
    }
    H($p, il);
    q = $p.prototype;
    q.sb = function() {
        return this.A ? this.A.sb() : 0
    };

    function aq(a, b) {
        a.A = b;
        for (Uo(a.A, "TileReady", a); 0 < a.B.length;) a.B.shift()()
    }
    q.Da = function() {
        return this.A ? this.A.Da() : null
    };
    q.fa = function() {
        return this.A ? this.A.fa() : null
    };
    q.Rb = function(a) {
        if (this.A) this.A.Rb(a);
        else {
            var b = this;
            this.B.push(a.sa(function() {
                b.A.Rb(a)
            }, "dr-prefetch"))
        }
    };
    q.ma = function() {
        return this.A ? this.A.ma() : null
    };
    q.Jb = function() {
        if (this.A) this.A.Jb();
        else {
            var a = this;
            this.B.push(function() {
                a.A.Jb()
            })
        }
    };
    q.Ib = function(a) {
        if (this.A) this.A.Ib(a);
        else {
            var b = this;
            this.B.push(function() {
                b.A.Ib(a)
            })
        }
    };
    q.Uc = function(a) {
        return this.A ? this.A.Uc(a) : !1
    };
    q.tb = function(a, b, c) {
        return this.A ? this.A.tb(a, b, c) : null
    };
    q.Hd = function(a, b) {
        if (this.A) this.A.Hd(a, b);
        else {
            var c = this;
            this.B.push(function() {
                c.A.Hd(a, b)
            })
        }
    };
    q.Gd = function(a, b) {
        if (this.A) this.A.Gd(a, b);
        else {
            var c = this;
            this.B.push(function() {
                c.A.Gd(a, b)
            })
        }
    };
    q.Ac = function(a, b) {
        if (this.A) this.A.Ac(a, b);
        else {
            var c = this;
            this.B.push(a.sa(function() {
                c.A.Ac(a, b)
            }, "dr-getconfig"))
        }
    };
    q.Tb = function(a, b) {
        if (this.A) this.A.Tb(a, b);
        else {
            var c = this;
            this.B.push(b.sa(function() {
                c.A.Tb(a, b)
            }, "dr-setconfig"))
        }
    };
    q.he = function() {
        if (this.A) this.A.he();
        else {
            var a = this;
            this.B.push(function() {
                a.A.he()
            })
        }
    };
    q.Gb = function(a, b, c, d, e) {
        if (this.A) this.A.Gb(a, b, c, d, e);
        else {
            var f = this;
            this.B.push(d.sa(function() {
                f.A.Gb(a, b, c, d, e)
            }, "dr-getile"))
        }
    };
    q.Fc = function(a, b, c) {
        if (this.A) this.A.Fc(a, b, c);
        else {
            var d = this;
            this.B.push(function() {
                d.A.Fc(a, b, c)
            })
        }
    };
    q.Dd = function(a, b, c) {
        if (this.A) this.A.Dd(a, b, c);
        else {
            var d = this;
            this.B.push(function() {
                d.A.Dd(a, b, c)
            })
        }
    };
    q.Je = function() {
        return this.A ? this.A.Je() : !1
    };
    q.Kc = function() {
        if (this.A) this.A.Kc();
        else {
            var a = this;
            this.B.push(function() {
                a.A.Kc()
            })
        }
    };
    q.Le = function() {
        return this.A ? this.A.Le() : !1
    };
    q.Qd = function(a, b) {
        return this.A ? this.A.Qd(a, b) : null
    };
    q.Nb = function() {
        return this.A ? this.A.Nb() : []
    };
    q.je = function(a) {
        if (this.A) this.A.je(a);
        else {
            var b = this;
            this.B.push(function() {
                b.A.je(a)
            })
        }
    };
    q.id = h("C");
    q.ia = function() {
        return this.A ? this.A.ia() : null
    };
    q.dc = function(a) {
        if (this.A) this.A.dc(a);
        else {
            var b = this;
            this.B.push(function() {
                b.A.dc(a)
            })
        }
    };

    function bq() {
        $p.call(this)
    }
    H(bq, $p);
    bq.prototype.Ha = function() {
        return this.A ? this.A.Ha() : null
    };
    bq.prototype.Nb = function() {
        return this.A ? this.A.Nb() : []
    };
    bq.prototype.Pd = function() {
        return this.A ? this.A.Pd() : null
    };
    bq.prototype.Cb = function() {
        return this.A ? this.A.Cb() : null
    };

    function cq(a, b, c, d) {
        this.A = a;
        this.G = b;
        this.X = new Vo;
        this.C = c;
        this.L = null;
        this.ga = d;
        this.aa = this.P = this.$ = this.N = 1;
        this.ha = 0;
        this.K = this.F = null;
        this.M = [];
        this.B = [];
        this.J = ok();
        this.O = El();
        this.da = El();
        this.ja = {};
        this.D = {};
        this.I = [];
        this.H = 0;
        this.Y = !1;
        dq(this);
        this.U = new Rp(0, 0, 0, eq(this, 0, 0, 0), fq)
    }
    var fq = new Float64Array(6);
    fq.set([-Infinity, -Infinity, -Infinity, Infinity, Infinity, Infinity]);
    var gq = [Cl(), Cl(), Cl(), Cl(), Cl(), Cl()],
        Kl = El(),
        hq = rl(),
        iq = rl();

    function Sm(a) {
        tp(a);
        for (var b = [], c = 0; c < a.B.length; ++c) {
            var d = a.B[c];
            d.Ba() && b.push(d)
        }
        return b
    }

    function kp(a, b) {
        tp(a);
        for (var c = 0; c < a.B.length; ++c) {
            var d = a.B[c],
                e = d,
                f = b,
                g = e.J,
                k = Math.max(0, e.D.ma().A - g),
                l = e.G >> k,
                k = e.H >> k;
            e.D.Gb(l, k, g, f);
            e = a;
            g = l + "|" + k + "|" + g;
            e.A.Uc(g) || (e.D[g] || (e.D[g] = []), -1 == e.D[g].indexOf(d) && e.D[g].push(d))
        }
    }

    function np(a, b, c, d) {
        a.F && (a.F.cancel(b), a.F = null);
        tp(a);
        for (var e = [], f = 0; f < a.B.length; ++f) {
            var g = a.B[f];
            g.wg() && e.push(g)
        }
        e.length ? (a.F = new Wp(e, c, d), jq(b, a.F, kq(2, !1))) : d && d()
    }

    function mp(a, b, c, d) {
        a.K && (a.K.cancel(b), a.K = null);
        tp(a);
        b = [];
        for (var e = 0; e < a.B.length; ++e) {
            var f = a.B[e];
            f.Ba() || b.push(f)
        }
        if (b.length)
            for (a = new Yp(b, c, d), c = a.start(); c != rj;) c = c.apply(a);
        else d && d()
    }

    function xm(a) {
        tp(a);
        return a.J
    }

    function lq(a) {
        var b = a.A.ma();
        a.$ = b.C;
        a.aa = b.B;
        a.N = Math.ceil(a.$);
        a.P = Math.ceil(a.aa);
        Sp(a.U, a.N, a.P, b.A, function(b, d, e) {
            return eq(a, b, d, e)
        });
        a.ha = a.C.J;
        a.Y = !0
    }

    function tp(a) {
        if (!a.Y) {
            if (!a.A.ma() || !a.A.ma().H) return;
            lq(a)
        }
        var b = !Oc(Wo(a.X), Wo(a.G)) || !a.L,
            c;
        if (!(c = !a.L)) {
            c = a.L;
            var d = a.C;
            c = !(c.D === d.D && c.F === d.F && c.G === d.G && c.I === d.I && c.A === d.A && c.B === d.B && c.C === d.C && c.O === d.O && c.P === d.P && c.U === d.U && c.J === d.J && c.M === d.M && c.K === d.K && c.W() === d.W() && c.H === d.H)
        }
        if (c || b) {
            c = a.C;
            d = new zm;
            lm(d, nm(c));
            d.Y = c.Y;
            a.L = d;
            U(a.X, a.G);
            if (b) {
                var b = a.A.ma(),
                    e;
                K(a.G, 2) && (e = Wo(a.G));
                Np(b, a.O, e);
                Nl(a.O, a.da)
            }
            e = a.J;
            b = a.C;
            c = a.O;
            var d = b.L,
                f = 1 / b.I,
                g = b.L;
            g[0] = f;
            g[1] = 0;
            g[2] = 0;
            g[3] = 0;
            g[4] = 0;
            g[5] = f;
            g[6] = 0;
            g[7] = 0;
            g[8] = 0;
            g[9] = 0;
            g[10] = f;
            g[11] = 0;
            g[12] = 0;
            g[13] = 0;
            g[14] = 0;
            g[15] = 1;
            Tl(g, -b.D, -b.F, -b.G);
            Ml(g, c, d);
            pk(e, b.L);
            qk(Bm(a.C), e, a.J);
            e = a.J;
            a.A.ma();
            a.J = e;
            mq(a)
        }
    }

    function mq(a) {
        nq(a);
        var b = a.A instanceof bq && oq(a),
            c = a.A.ma();
        a.B = Up(a.U, gq, b, a.N, a.P, c.A, function(b, c, f) {
            return eq(a, b, c, f)
        }, function(b, e) {
            var d = a.C,
                g = a.O,
                k = oq(a),
                l = un(c, e),
                m = un(c, e + 1);
            k ? g = (m - l) * Math.PI : (k = tn(c, b), b = tn(c, b + 1), b = (k + b) / 2, zn(c, b, l, pq, 0), zn(c, b, m, qq, 0), l = qq, Ol(g, pq, rq), Ol(g, l, sq), rq[0] -= d.A, rq[1] -= d.B, rq[2] -= d.C, sq[0] -= d.A, sq[1] -= d.B, sq[2] -= d.C, g = Math.acos(zl(rq, sq) / (xl(rq) * xl(sq))));
            l = d.J;
            g = 1E-6 < Math.abs(l) ? g * d.H / l : 0;
            d = c.A;
            l = c.B - (c.G - 1);
            1 > l && e == c.G - 1 && (g /= l);
            e = Math.floor(Math.log(Ce(c.D) /
                g) / Math.LN2);
            e = Eb(e, 0, d);
            return Math.max(2, d - e)
        })
    }

    function eq(a, b, c, d) {
        var e = b + c * a.N;
        d >= a.M.length && (a.M[d] = []);
        a.M[d][e] || (a.M[d][e] = a.ga.create(a.A, a.ja, b, c, d));
        return a.M[d][e]
    }

    function dq(a) {
        Qo(a.A, "TileReady", function(b, c) {
            b = c.x + "|" + c.y + "|" + c.zoom;
            if (a.D[b]) {
                c = 0;
                for (var d = a.D[b].length; c < d; c++) {
                    var e = a.D[b][c];
                    1 == e.P && (e.P = 0, a.I.push(e))
                }
                delete a.D[b];
                if (!a.H) {
                    b = 0;
                    for (var f in a.D) b++;
                    5 >= b && (a.H = F())
                }
            }
        })
    }

    function oq(a) {
        var b;
        b = K(a.G, 2) ? qe(Wo(a.G)) : qe(a.A.fa());
        dm(we(b), xe(b), ye(b), hq);
        bm(a.C.A, a.C.B, a.C.C, iq);
        Zl(iq[0], iq[1], iq[2], iq);
        return .01 > Math.sqrt(Bl(iq, hq))
    }

    function nq(a) {
        Gl(Kl, xm(a));
        Jl(3, kn);
        kn[0] = -kn[0];
        kn[1] = -kn[1];
        kn[2] = -kn[2];
        kn[3] = -kn[3];
        for (a = 0; 3 > a; a++) {
            var b = 2 * a;
            Jl(a, ln);
            var c = gq[b],
                d = c,
                e = kn,
                f = ln;
            d[0] = e[0] - f[0];
            d[1] = e[1] - f[1];
            d[2] = e[2] - f[2];
            d[3] = e[3] - f[3];
            Dl(c, 1 / Math.sqrt(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]), c);
            c = b = gq[b + 1];
            d = kn;
            e = ln;
            c[0] = d[0] + e[0];
            c[1] = d[1] + e[1];
            c[2] = d[2] + e[2];
            c[3] = d[3] + e[3];
            Dl(b, 1 / Math.sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]), b)
        }
    }

    function tq(a) {
        this.A = a
    }
    tq.prototype.create = function(a, b, c) {
        return new cq(a, b, c, this.A)
    };

    function uq(a, b, c, d) {
        b = new Em(b, c);
        ep.call(this, a, b, new tq(new Mn), d)
    }
    H(uq, Lp);

    function vq(a, b, c, d, e, f) {
        b = new uq(c, d, e, f);
        a(b)
    };

    function wq(a) {
        Bk.call(this);
        this.F = a;
        this.A = {}
    }
    H(wq, Bk);
    var xq = [];
    q = wq.prototype;
    q.listen = function(a, b, c, d) {
        return yq(this, a, b, c, d)
    };

    function yq(a, b, c, d, e, f) {
        ua(c) || (c && (xq[0] = c.toString()), c = xq);
        for (var g = 0; g < c.length; g++) {
            var k = Vk(b, c[g], d || a.handleEvent, e || !1, f || a.F || a);
            if (!k) break;
            a.A[k.key] = k
        }
        return a
    }
    q.Cg = function(a, b, c, d) {
        return zq(this, a, b, c, d)
    };

    function zq(a, b, c, d, e, f) {
        if (ua(c))
            for (var g = 0; g < c.length; g++) zq(a, b, c[g], d, e, f);
        else {
            b = bl(b, c, d || a.handleEvent, e, f || a.F || a);
            if (!b) return a;
            a.A[b.key] = b
        }
        return a
    }
    q.Ub = function(a, b, c, d, e) {
        if (ua(b))
            for (var f = 0; f < b.length; f++) this.Ub(a, b[f], c, d, e);
        else c = c || this.handleEvent, e = e || this.F || this, c = Wk(c), d = !!d, b = zk(a) ? a.Qc(b, c, d, e) : a ? (a = Yk(a)) ? a.Qc(b, c, d, e) : null : null, b && (dl(b), delete this.A[b.key]);
        return this
    };

    function Aq(a) {
        Mb(a.A, function(a, c) {
            this.A.hasOwnProperty(c) && dl(a)
        }, a);
        a.A = {}
    }
    q.ea = function() {
        wq.V.ea.call(this);
        Aq(this)
    };
    q.handleEvent = function() {
        throw Error("EventHandler.handleEvent not implemented");
    };

    function Bq(a, b, c) {
        this.F = c;
        this.C = a;
        this.D = b;
        this.B = 0;
        this.A = null
    }
    Bq.prototype.get = function() {
        var a;
        0 < this.B ? (this.B--, a = this.A, this.A = a.next, a.next = null) : a = this.C();
        return a
    };

    function Cq(a, b) {
        a.D(b);
        a.B < a.F && (a.B++, b.next = a.A, a.A = b)
    };

    function Dq(a) {
        w.setTimeout(function() {
            throw a;
        }, 0)
    }

    function Eq(a) {
        !ya(w.setImmediate) || w.Window && w.Window.prototype && !Yb("Edge") && w.Window.prototype.setImmediate == w.setImmediate ? (Fq || (Fq = Gq()), Fq(a)) : w.setImmediate(a)
    }
    var Fq;

    function Gq() {
        var a = w.MessageChannel;
        "undefined" === typeof a && "undefined" !== typeof window && window.postMessage && window.addEventListener && !Yb("Presto") && (a = function() {
            var a = document.createElement("IFRAME");
            a.style.display = "none";
            a.src = "";
            document.documentElement.appendChild(a);
            var b = a.contentWindow,
                a = b.document;
            a.open();
            a.write("");
            a.close();
            var c = "callImmediate" + Math.random(),
                d = "file:" == b.location.protocol ? "*" : b.location.protocol + "//" + b.location.host,
                a = E(function(a) {
                    if (("*" == d || a.origin == d) && a.data ==
                        c) this.port1.onmessage()
                }, this);
            b.addEventListener("message", a, !1);
            this.port1 = {};
            this.port2 = {
                postMessage: function() {
                    b.postMessage(c, d)
                }
            }
        });
        if ("undefined" !== typeof a && !zc()) {
            var b = new a,
                c = {},
                d = c;
            b.port1.onmessage = function() {
                if (y(c.next)) {
                    c = c.next;
                    var a = c.rf;
                    c.rf = null;
                    a()
                }
            };
            return function(a) {
                d.next = {
                    rf: a
                };
                d = d.next;
                b.port2.postMessage(0)
            }
        }
        return "undefined" !== typeof document && "onreadystatechange" in document.createElement("SCRIPT") ? function(a) {
            var b = document.createElement("SCRIPT");
            b.onreadystatechange =
                function() {
                    b.onreadystatechange = null;
                    b.parentNode.removeChild(b);
                    b = null;
                    a();
                    a = null
                };
            document.documentElement.appendChild(b)
        } : function(a) {
            w.setTimeout(a, 0)
        }
    };

    function Hq() {
        this.B = this.A = null
    }
    var Jq = new Bq(function() {
        return new Iq
    }, function(a) {
        a.reset()
    }, 100);
    Hq.prototype.add = function(a, b) {
        var c = Jq.get();
        c.set(a, b);
        this.B ? this.B.next = c : this.A = c;
        this.B = c
    };
    Hq.prototype.remove = function() {
        var a = null;
        this.A && (a = this.A, this.A = this.A.next, this.A || (this.B = null), a.next = null);
        return a
    };

    function Iq() {
        this.next = this.scope = this.Oc = null
    }
    Iq.prototype.set = function(a, b) {
        this.Oc = a;
        this.scope = b;
        this.next = null
    };
    Iq.prototype.reset = function() {
        this.next = this.scope = this.Oc = null
    };

    function Kq(a, b) {
        Lq || Mq();
        Nq || (Lq(), Nq = !0);
        Oq.add(a, b)
    }
    var Lq;

    function Mq() {
        if (-1 != String(w.Promise).indexOf("[native code]")) {
            var a = w.Promise.resolve(void 0);
            Lq = function() {
                a.then(Pq)
            }
        } else Lq = function() {
            Eq(Pq)
        }
    }
    var Nq = !1,
        Oq = new Hq;

    function Pq() {
        for (var a; a = Oq.remove();) {
            try {
                a.Oc.call(a.scope)
            } catch (b) {
                Dq(b)
            }
            Cq(Jq, a)
        }
        Nq = !1
    };

    function Qq(a) {
        a.prototype.then = a.prototype.then;
        a.prototype.$goog_Thenable = !0
    }

    function Rq(a) {
        if (!a) return !1;
        try {
            return !!a.$goog_Thenable
        } catch (b) {
            return !1
        }
    };

    function Sq(a, b) {
        this.A = 0;
        this.H = void 0;
        this.D = this.B = this.C = null;
        this.F = this.G = !1;
        if (a != C) try {
            var c = this;
            a.call(b, function(a) {
                Tq(c, 2, a)
            }, function(a) {
                Tq(c, 3, a)
            })
        } catch (d) {
            Tq(this, 3, d)
        }
    }

    function Uq() {
        this.next = this.C = this.B = this.D = this.A = null;
        this.F = !1
    }
    Uq.prototype.reset = function() {
        this.C = this.B = this.D = this.A = null;
        this.F = !1
    };
    var Vq = new Bq(function() {
        return new Uq
    }, function(a) {
        a.reset()
    }, 100);

    function Wq(a, b, c) {
        var d = Vq.get();
        d.D = a;
        d.B = b;
        d.C = c;
        return d
    }
    Sq.prototype.then = function(a, b, c) {
        return Xq(this, ya(a) ? a : null, ya(b) ? b : null, c)
    };
    Qq(Sq);
    Sq.prototype.cancel = function(a) {
        0 == this.A && Kq(function() {
            var b = new Yq(a);
            Zq(this, b)
        }, this)
    };

    function Zq(a, b) {
        if (0 == a.A)
            if (a.C) {
                var c = a.C;
                if (c.B) {
                    for (var d = 0, e = null, f = null, g = c.B; g && (g.F || (d++, g.A == a && (e = g), !(e && 1 < d))); g = g.next) e || (f = g);
                    e && (0 == c.A && 1 == d ? Zq(c, b) : (f ? (d = f, d.next == c.D && (c.D = d), d.next = d.next.next) : $q(c), ar(c, e, 3, b)))
                }
                a.C = null
            } else Tq(a, 3, b)
    }

    function br(a, b) {
        a.B || 2 != a.A && 3 != a.A || cr(a);
        a.D ? a.D.next = b : a.B = b;
        a.D = b
    }

    function Xq(a, b, c, d) {
        var e = Wq(null, null, null);
        e.A = new Sq(function(a, g) {
            e.D = b ? function(c) {
                try {
                    var e = b.call(d, c);
                    a(e)
                } catch (m) {
                    g(m)
                }
            } : a;
            e.B = c ? function(b) {
                try {
                    var e = c.call(d, b);
                    !y(e) && b instanceof Yq ? g(b) : a(e)
                } catch (m) {
                    g(m)
                }
            } : g
        });
        e.A.C = a;
        br(a, e);
        return e.A
    }
    Sq.prototype.K = function(a) {
        this.A = 0;
        Tq(this, 2, a)
    };
    Sq.prototype.J = function(a) {
        this.A = 0;
        Tq(this, 3, a)
    };

    function Tq(a, b, c) {
        if (0 == a.A) {
            a === c && (b = 3, c = new TypeError("Promise cannot resolve to itself"));
            a.A = 1;
            var d;
            a: {
                var e = c,
                    f = a.K,
                    g = a.J;
                if (e instanceof Sq) br(e, Wq(f || C, g || null, a)),
                d = !0;
                else if (Rq(e)) e.then(f, g, a),
                d = !0;
                else {
                    if (za(e)) try {
                        var k = e.then;
                        if (ya(k)) {
                            dr(e, k, f, g, a);
                            d = !0;
                            break a
                        }
                    } catch (l) {
                        g.call(a, l);
                        d = !0;
                        break a
                    }
                    d = !1
                }
            }
            d || (a.H = c, a.A = b, a.C = null, cr(a), 3 != b || c instanceof Yq || er(a, c))
        }
    }

    function dr(a, b, c, d, e) {
        function f(a) {
            k || (k = !0, d.call(e, a))
        }

        function g(a) {
            k || (k = !0, c.call(e, a))
        }
        var k = !1;
        try {
            b.call(a, g, f)
        } catch (l) {
            f(l)
        }
    }

    function cr(a) {
        a.G || (a.G = !0, Kq(a.I, a))
    }

    function $q(a) {
        var b = null;
        a.B && (b = a.B, a.B = b.next, b.next = null);
        a.B || (a.D = null);
        return b
    }
    Sq.prototype.I = function() {
        for (var a; a = $q(this);) ar(this, a, this.A, this.H);
        this.G = !1
    };

    function ar(a, b, c, d) {
        if (3 == c && b.B && !b.F)
            for (; a && a.F; a = a.C) a.F = !1;
        if (b.A) b.A.C = null, fr(b, c, d);
        else try {
            b.F ? b.D.call(b.C) : fr(b, c, d)
        } catch (e) {
            gr.call(null, e)
        }
        Cq(Vq, b)
    }

    function fr(a, b, c) {
        2 == b ? a.D.call(a.C, c) : a.B && a.B.call(a.C, c)
    }

    function er(a, b) {
        a.F = !0;
        Kq(function() {
            a.F && gr.call(null, b)
        })
    }
    var gr = Dq;

    function Yq(a) {
        Ha.call(this, a)
    }
    H(Yq, Ha);
    Yq.prototype.name = "cancel";

    function hr(a, b) {
        return new Sq(function(c, d) {
            var e;
            e = y(b) ? ya(b) ? b() : b : new Image;
            var f = Rc && 11 > dd ? "readystatechange" : "load",
                g = new wq;
            g.listen(e, [f, "abort", "error"], function(a) {
                if ("readystatechange" != a.type || "complete" == e.readyState) Dk(g), a.type == f ? c(e) : d(null)
            });
            e.src = a
        })
    };
    var ir = /^(?:([^:/?#.]+):)?(?:\/\/(?:([^/?#]*)@)?([^/#?]*?)(?::([0-9]+))?(?=[/#?]|$))?([^?#]+)?(?:\?([^#]*))?(?:#([\s\S]*))?$/;

    function jr(a, b) {
        if (a) {
            a = a.split("&");
            for (var c = 0; c < a.length; c++) {
                var d = a[c].indexOf("="),
                    e, f = null;
                0 <= d ? (e = a[c].substring(0, d), f = a[c].substring(d + 1)) : e = a[c];
                b(e, f ? decodeURIComponent(f.replace(/\+/g, " ")) : "")
            }
        }
    }

    function kr(a, b, c) {
        if (ua(b))
            for (var d = 0; d < b.length; d++) kr(a, String(b[d]), c);
        else null != b && c.push("&", a, "" === b ? "" : "=", encodeURIComponent(String(b)))
    };

    function lr(a, b) {
        this.C = this.I = this.B = "";
        this.H = null;
        this.F = this.D = "";
        this.G = !1;
        var c;
        a instanceof lr ? (this.G = y(b) ? b : a.G, mr(this, a.B), this.I = a.I, this.C = a.C, nr(this, a.H), this.D = a.D, b = a.A, c = new or, c.B = b.B, b.A && (c.A = new $i(b.A), c.ca = b.ca), pr(this, c), this.F = a.F) : a && (c = String(a).match(ir)) ? (this.G = !!b, mr(this, c[1] || "", !0), this.I = qr(c[2] || ""), this.C = qr(c[3] || "", !0), nr(this, c[4]), this.D = qr(c[5] || "", !0), pr(this, c[6] || "", !0), this.F = qr(c[7] || "")) : (this.G = !!b, this.A = new or(null, 0, this.G))
    }
    lr.prototype.toString = function() {
        var a = [],
            b = this.B;
        b && a.push(rr(b, sr, !0), ":");
        var c = this.C;
        if (c || "file" == b) a.push("//"), (b = this.I) && a.push(rr(b, sr, !0), "@"), a.push(encodeURIComponent(String(c)).replace(/%25([0-9a-fA-F]{2})/g, "%$1")), c = this.H, null != c && a.push(":", String(c));
        if (c = this.D) this.C && "/" != c.charAt(0) && a.push("/"), a.push(rr(c, "/" == c.charAt(0) ? tr : ur, !0));
        (c = this.A.toString()) && a.push("?", c);
        (c = this.F) && a.push("#", rr(c, vr));
        return a.join("")
    };

    function mr(a, b, c) {
        a.B = c ? qr(b, !0) : b;
        a.B && (a.B = a.B.replace(/:$/, ""))
    }

    function nr(a, b) {
        if (b) {
            b = Number(b);
            if (isNaN(b) || 0 > b) throw Error("Bad port number " + b);
            a.H = b
        } else a.H = null
    }

    function pr(a, b, c) {
        b instanceof or ? (a.A = b, wr(a.A, a.G)) : (c || (b = rr(b, xr)), a.A = new or(b, 0, a.G))
    }

    function qr(a, b) {
        return a ? b ? decodeURI(a.replace(/%25/g, "%2525")) : decodeURIComponent(a) : ""
    }

    function rr(a, b, c) {
        return wa(a) ? (a = encodeURI(a).replace(b, yr), c && (a = a.replace(/%25([0-9a-fA-F]{2})/g, "%$1")), a) : null
    }

    function yr(a) {
        a = a.charCodeAt(0);
        return "%" + (a >> 4 & 15).toString(16) + (a & 15).toString(16)
    }
    var sr = /[#\/\?@]/g,
        ur = /[\#\?:]/g,
        tr = /[\#\?]/g,
        xr = /[\#\?@]/g,
        vr = /#/g;

    function or(a, b, c) {
        this.ca = this.A = null;
        this.B = a || null;
        this.C = !!c
    }

    function zr(a) {
        a.A || (a.A = new $i, a.ca = 0, a.B && jr(a.B, function(b, c) {
            a.add(decodeURIComponent(b.replace(/\+/g, " ")), c)
        }))
    }
    q = or.prototype;
    q.Bb = function() {
        zr(this);
        return this.ca
    };
    q.add = function(a, b) {
        zr(this);
        this.B = null;
        a = Ar(this, a);
        var c = this.A.get(a);
        c || this.A.set(a, c = []);
        c.push(b);
        this.ca = this.ca + 1;
        return this
    };
    q.remove = function(a) {
        zr(this);
        a = Ar(this, a);
        return bj(this.A.B, a) ? (this.B = null, this.ca = this.ca - this.A.get(a).length, this.A.remove(a)) : !1
    };
    q.clear = function() {
        this.A = this.B = null;
        this.ca = 0
    };
    q.Oa = function() {
        zr(this);
        return 0 == this.ca
    };

    function Br(a, b) {
        zr(a);
        b = Ar(a, b);
        return bj(a.A.B, b)
    }
    q.kb = function() {
        zr(this);
        for (var a = this.A.Ca(), b = this.A.kb(), c = [], d = 0; d < b.length; d++)
            for (var e = a[d], f = 0; f < e.length; f++) c.push(b[d]);
        return c
    };
    q.Ca = function(a) {
        zr(this);
        var b = [];
        if (wa(a)) Br(this, a) && (b = vb(b, this.A.get(Ar(this, a))));
        else {
            a = this.A.Ca();
            for (var c = 0; c < a.length; c++) b = vb(b, a[c])
        }
        return b
    };
    q.set = function(a, b) {
        zr(this);
        this.B = null;
        a = Ar(this, a);
        Br(this, a) && (this.ca = this.ca - this.A.get(a).length);
        this.A.set(a, [b]);
        this.ca = this.ca + 1;
        return this
    };
    q.get = function(a, b) {
        a = a ? this.Ca(a) : [];
        return 0 < a.length ? String(a[0]) : b
    };

    function Cr(a, b, c) {
        a.remove(b);
        0 < c.length && (a.B = null, a.A.set(Ar(a, b), wb(c)), a.ca = a.ca + c.length)
    }
    q.toString = function() {
        if (this.B) return this.B;
        if (!this.A) return "";
        for (var a = [], b = this.A.kb(), c = 0; c < b.length; c++)
            for (var d = b[c], e = encodeURIComponent(String(d)), d = this.Ca(d), f = 0; f < d.length; f++) {
                var g = e;
                "" !== d[f] && (g += "=" + encodeURIComponent(String(d[f])));
                a.push(g)
            }
        return this.B = a.join("&")
    };

    function Ar(a, b) {
        b = String(b);
        a.C && (b = b.toLowerCase());
        return b
    }

    function wr(a, b) {
        b && !a.C && (zr(a), a.B = null, a.A.forEach(function(a, b) {
            var c = b.toLowerCase();
            b != c && (this.remove(b), Cr(this, c, a))
        }, a));
        a.C = b
    };

    function Dr(a) {
        this.B = !1;
        this.C = a
    }
    Dr.prototype.A = function() {
        this.B || (this.B = !0, this.C())
    };

    function Er() {}
    H(Er, Error);

    function Fr() {
        this.A = "pending";
        this.C = [];
        this.B = this.D = void 0
    }
    Qq(Fr);

    function Gr() {
        Ha.call(this, "Multiple attempts to set the state of this Result")
    }
    H(Gr, Ha);
    q = Fr.prototype;
    q.getError = h("B");

    function Hr(a, b) {
        "pending" == a.A ? a.C.push({
            sa: b,
            scope: null
        }) : b.call(void 0, a)
    }
    q.Ik = function(a) {
        if ("pending" == this.A) this.D = a, this.A = "success", Ir(this);
        else if (!Jr(this)) throw new Gr;
    };
    q.qh = function(a) {
        if ("pending" == this.A) this.B = a, this.A = "error", Ir(this);
        else if (!Jr(this)) throw new Gr;
    };

    function Ir(a) {
        var b = a.C;
        a.C = [];
        for (var c = 0; c < b.length; c++) {
            var d = b[c];
            d.sa.call(d.scope, a)
        }
    }
    q.cancel = function() {
        return "pending" == this.A ? (this.qh(new Er), !0) : !1
    };

    function Jr(a) {
        return "error" == a.A && a.B instanceof Er
    }
    q.then = function(a, b, c) {
        var d, e, f = new Sq(function(a, b) {
            d = a;
            e = b
        });
        Hr(this, function(a) {
            Jr(a) ? f.cancel() : "success" == a.A ? d(a.D) : "error" == a.A && e(a.getError())
        });
        return f.then(a, b, c)
    };

    function Kr(a) {
        var b = new Fr;
        a.then(b.Ik, b.qh, b);
        return b
    };

    function Lr(a, b) {
        this.B = a || Mr;
        this.C = b || C
    }
    Lr.prototype.A = function(a, b) {
        a = new lr(a);
        var c = Kr(hr(a.toString(), Fa(this.B, !a.C, !!a.B && "data" === a.B))),
            d = this;
        Hr(c, function(a) {
            try {
                b(a.D)
            } catch (f) {
                throw d.C(f), f;
            }
        });
        return new Dr(function() {
            c.cancel()
        })
    };

    function Mr(a, b) {
        var c = Rj("IMG");
        a || b || (c.crossOrigin = "");
        return c
    };

    function Nr(a, b) {
        this.B = a;
        this.C = b
    }
    Nr.prototype.A = function(a, b, c) {
        b = new Or(this.B, b);
        a = new Pr(a, E(b.C, b), this.C);
        b.B = a;
        Qr(this.B, a, c || 2);
        return b
    };

    function Or(a, b) {
        this.G = a;
        this.F = b;
        this.D = !1;
        this.B = null
    }
    Or.prototype.C = function(a) {
        this.D || (this.F(a), this.D = !0)
    };
    Or.prototype.A = function() {
        this.B && (this.G.remove(this.B), this.C(void 0))
    };

    function Pr(a, b, c) {
        this.C = a;
        this.A = b;
        this.B = c;
        this.state = null
    }
    Pr.prototype.start = function(a) {
        var b = this.A;
        this.B.A(this.C, function(c) {
            a();
            b(c)
        })
    };
    Pr.prototype.cancel = function() {
        this.A(void 0);
        return !1
    };

    function Rr(a, b) {
        this.A = new Nr(a, b ? b : new Lr)
    }
    Rr.prototype.Ja = function(a, b, c) {
        if (!a) return b(null), C;
        a = this.A.A(a, function(a) {
            b(a)
        }, c || 3);
        return E(a.A, a)
    };
    var Sr = null;

    function Tr(a) {
        return Sr ? null != Sr.Pg(a) : !1
    };

    function Ur(a, b) {
        this.B = b;
        this.A = new Rr(a, new Lr(function() {
            return Rj("IMG")
        }))
    }
    Ur.prototype.tb = function(a, b, c, d, e, f, g) {
        a = Sr.Tm(this.B, d, b, c);
        return a ? this.A.Ja(a, f.sa(e, "custom-pano-tile"), g) : (e(null), C)
    };
    Ur.prototype.Tb = aa();

    function Vr(a, b) {
        V.call(this, "CUTS", wb(arguments))
    }
    H(Vr, V);

    function Wr(a, b, c, d) {
        a(new Ur(c, d))
    };

    function Xr(a) {
        this.B = a;
        this.A = Math.ceil(3 * a.length / 4);
        if (!y(Yr[0])) {
            for (a = 0; a < Yr.length; a++) Yr[a] = Math.pow(2, a - 150);
            for (a = 0; 65 > a; a++) Zr["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=".charCodeAt(a)] = a;
            for (a = 0; 65 > a; a++) $r["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.".charCodeAt(a)] = a
        }
    }
    var Yr = Array(256),
        Zr = Array(64),
        $r = Array(64);

    function as(a, b, c) {
        var d = Array(c),
            e = 0;
        b += Math.floor(b / 3);
        c = Math.ceil(4 * c / 3);
        var f = a.B.length;
        f - b < c && (c = f - b);
        for (var f = $r, g = b - b % 4; g < b + c; g += 4) {
            var k = e,
                l = g >= b,
                k = k + (l ? 1 : 0),
                m = g + 1 >= b && k < d.length,
                k = k + (m ? 1 : 0),
                k = g + 2 >= b && k < d.length;
            if (l) {
                var n = f[a.B.charCodeAt(g + 0)];
                if (!y(n)) return null
            }
            if (l || m) {
                var p = f[a.B.charCodeAt(g + 1)];
                if (!y(p)) return null
            }
            if (m || k) {
                var r = f[a.B.charCodeAt(g + 2)];
                if (!y(r)) return null
            }
            if (k) {
                var u = f[a.B.charCodeAt(g + 3)];
                if (!y(u)) return null
            }
            l && 64 != n && 64 != p && (d[e++] = n << 2 | p >> 4);
            m && 64 != p && 64 !=
                r && (d[e++] = p << 4 & 240 | r >> 2);
            k && 64 != r && 64 != u && (d[e++] = r << 6 & 192 | u)
        }
        d.length = e;
        return d
    }

    function Kp(a, b) {
        if (!(0 > b || b > a.A - 1) && (a = as(a, b, 1)) && 1 == a.length) return a[0]
    }

    function bs(a, b) {
        if (!(0 > b || b > a.A - 2 || (a = as(a, b, 2), 2 > a.length))) return a[0] + (a[1] << 8)
    }

    function cs(a, b, c) {
        if (!a.A) return [];
        var d = 4 * c;
        if (0 > b || b + d > a.A) return [];
        c = Array(c);
        a = as(a, b, d);
        for (b = 0; b < c.length; b++) d = 4 * b, c[b] = 0 == (a[d + 3] & 127 | a[d + 2] | a[d + 1] | a[d]) ? 0 : (1 - ((a[d + 3] & 128) >> 6)) * ((a[d + 2] | 128) << 16 | a[d + 1] << 8 | a[d]) * Yr[(a[d + 3] & 127) << 1 | (a[d + 2] & 128) >> 7];
        return c
    };

    function ds(a) {
        this.C = null;
        this.G = this.D = this.F = 0;
        this.B = null;
        this.A = new Xr(a)
    }
    q = ds.prototype;
    q.$d = ba("C");
    q.Zd = ca(1);
    q.start = function() {
        return this.C ? this.il : rj
    };
    q.il = function() {
        if (8 != Kp(this.A, 0) || 8 != Kp(this.A, 7)) {
            var a = this.C;
            a.C = Fp;
            a.Jb();
            return rj
        }
        this.G = bs(this.A, 1) || 0;
        this.F = bs(this.A, 3) || 0;
        this.D = bs(this.A, 5) || 0;
        this.H = a = Kp(this.A, 7) || 0;
        a += this.F * this.D;
        this.B = cs(this.A, a, 4 * this.G);
        for (var a = 0, b = this.B.length; a < b; ++a) this.B[a] *= -1;
        return this.Bk
    };
    q.Bk = function() {
        var a = this.C;
        a.C = new zp(this.F, this.D, this.B, this.A, this.H);
        a.Jb();
        return rj
    };

    function es() {
        this.C = null;
        this.B = [];
        this.A = null
    };
    var fs = {
            Yh: ["BC", "AD"],
            Xh: ["Before Christ", "Anno Domini"],
            bi: "JFMAMJJASOND".split(""),
            ji: "JFMAMJJASOND".split(""),
            ai: "January February March April May June July August September October November December".split(" "),
            ii: "January February March April May June July August September October November December".split(" "),
            fi: "Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec".split(" "),
            li: "Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec".split(" "),
            oi: "Sunday Monday Tuesday Wednesday Thursday Friday Saturday".split(" "),
            ni: "Sunday Monday Tuesday Wednesday Thursday Friday Saturday".split(" "),
            hi: "Sun Mon Tue Wed Thu Fri Sat".split(" "),
            mi: "Sun Mon Tue Wed Thu Fri Sat".split(" "),
            bn: "SMTWTFS".split(""),
            ki: "SMTWTFS".split(""),
            gi: ["Q1", "Q2", "Q3", "Q4"],
            di: ["1st quarter", "2nd quarter", "3rd quarter", "4th quarter"],
            Rh: ["AM", "PM"],
            gf: ["EEEE, MMMM d, y", "MMMM d, y", "MMM d, y", "M/d/yy"],
            jf: ["h:mm:ss a zzzz", "h:mm:ss a z", "h:mm:ss a", "h:mm a"],
            Sh: ["{1} 'at' {0}", "{1} 'at' {0}", "{1}, {0}", "{1}, {0}"],
            Zh: 6,
            dn: [5, 6],
            $h: 5
        },
        gs = fs,
        gs = fs;

    function hs(a, b, c) {
        xa(a) ? (this.A = is(a, b || 0, c || 1), js(this, c || 1)) : za(a) ? (this.A = is(a.getFullYear(), a.getMonth(), a.getDate()), js(this, a.getDate())) : (this.A = new Date(F()), a = this.A.getDate(), this.A.setHours(0), this.A.setMinutes(0), this.A.setSeconds(0), this.A.setMilliseconds(0), js(this, a))
    }

    function is(a, b, c) {
        b = new Date(a, b, c);
        0 <= a && 100 > a && b.setFullYear(b.getFullYear() - 1900);
        return b
    }
    q = hs.prototype;
    q.getFullYear = function() {
        return this.A.getFullYear()
    };
    q.getMonth = function() {
        return this.A.getMonth()
    };
    q.getDate = function() {
        return this.A.getDate()
    };
    q.getTime = function() {
        return this.A.getTime()
    };
    q.getDay = function() {
        return this.A.getDay()
    };
    q.getUTCFullYear = function() {
        return this.A.getUTCFullYear()
    };
    q.getUTCMonth = function() {
        return this.A.getUTCMonth()
    };
    q.getUTCDate = function() {
        return this.A.getUTCDate()
    };
    q.getUTCHours = function() {
        return this.A.getUTCHours()
    };
    q.getUTCMinutes = function() {
        return this.A.getUTCMinutes()
    };
    q.getTimezoneOffset = function() {
        return this.A.getTimezoneOffset()
    };

    function ks(a) {
        a = a.getTimezoneOffset();
        if (0 == a) a = "Z";
        else {
            var b = Math.abs(a) / 60,
                c = Math.floor(b),
                b = 60 * (b - c);
            a = (0 < a ? "-" : "+") + bb(c, 2) + ":" + bb(b, 2)
        }
        return a
    }
    q.set = function(a) {
        this.A = new Date(a.getFullYear(), a.getMonth(), a.getDate())
    };
    q.add = function(a) {
        if (a.G || a.D) {
            var b = this.getMonth() + a.D + 12 * a.G,
                c = this.getFullYear() + Math.floor(b / 12),
                b = b % 12;
            0 > b && (b += 12);
            var d;
            a: {
                switch (b) {
                    case 1:
                        d = 0 != c % 4 || 0 == c % 100 && 0 != c % 400 ? 28 : 29;
                        break a;
                    case 5:
                    case 8:
                    case 10:
                    case 3:
                        d = 30;
                        break a
                }
                d = 31
            }
            d = Math.min(d, this.getDate());
            this.A.setDate(1);
            this.A.setFullYear(c);
            this.A.setMonth(b);
            this.A.setDate(d)
        }
        a.A && (a = new Date((new Date(this.getFullYear(), this.getMonth(), this.getDate(), 12)).getTime() + 864E5 * a.A), this.A.setDate(1), this.A.setFullYear(a.getFullYear()),
            this.A.setMonth(a.getMonth()), this.A.setDate(a.getDate()), js(this, a.getDate()))
    };
    q.ke = function(a, b) {
        return [this.getFullYear(), bb(this.getMonth() + 1, 2), bb(this.getDate(), 2)].join(a ? "-" : "") + (b ? ks(this) : "")
    };
    q.toString = function() {
        return this.ke()
    };

    function js(a, b) {
        a.getDate() != b && a.A.setUTCHours(a.A.getUTCHours() + (a.getDate() < b ? 1 : -1))
    }
    q.valueOf = function() {
        return this.A.valueOf()
    };

    function ls(a, b, c, d, e, f, g) {
        this.A = xa(a) ? new Date(a, b || 0, c || 1, d || 0, e || 0, f || 0, g || 0) : new Date(a && a.getTime ? a.getTime() : F())
    }
    H(ls, hs);
    q = ls.prototype;
    q.getHours = function() {
        return this.A.getHours()
    };
    q.getMinutes = function() {
        return this.A.getMinutes()
    };
    q.getSeconds = function() {
        return this.A.getSeconds()
    };
    q.getUTCHours = function() {
        return this.A.getUTCHours()
    };
    q.getUTCMinutes = function() {
        return this.A.getUTCMinutes()
    };
    q.add = function(a) {
        hs.prototype.add.call(this, a);
        a.B && this.A.setUTCHours(this.A.getUTCHours() + a.B);
        a.C && this.A.setUTCMinutes(this.A.getUTCMinutes() + a.C);
        a.F && this.A.setUTCSeconds(this.A.getUTCSeconds() + a.F)
    };
    q.ke = function(a, b) {
        var c = hs.prototype.ke.call(this, a);
        return a ? c + " " + bb(this.getHours(), 2) + ":" + bb(this.getMinutes(), 2) + ":" + bb(this.getSeconds(), 2) + (b ? ks(this) : "") : c + "T" + bb(this.getHours(), 2) + bb(this.getMinutes(), 2) + bb(this.getSeconds(), 2) + (b ? ks(this) : "")
    };
    q.toString = function() {
        return this.ke()
    };

    function ms() {}

    function ns(a) {
        if ("number" == typeof a) {
            var b = new ms;
            b.B = a;
            var c;
            c = a;
            if (0 == c) c = "Etc/GMT";
            else {
                var d = ["Etc/GMT", 0 > c ? "-" : "+"];
                c = Math.abs(c);
                d.push(Math.floor(c / 60) % 100);
                c %= 60;
                0 != c && d.push(":", bb(c, 2));
                c = d.join("")
            }
            b.D = c;
            c = a;
            0 == c ? c = "UTC" : (d = ["UTC", 0 > c ? "+" : "-"], c = Math.abs(c), d.push(Math.floor(c / 60) % 100), c %= 60, 0 != c && d.push(":", c), c = d.join(""));
            a = os(a);
            b.F = [c, c];
            b.A = {
                cn: a,
                hf: a
            };
            b.C = [];
            return b
        }
        b = new ms;
        b.D = a.id;
        b.B = -a.std_offset;
        b.F = a.names;
        b.A = a.names_ext;
        b.C = a.transitions;
        return b
    }

    function os(a) {
        var b = ["GMT"];
        b.push(0 >= a ? "+" : "-");
        a = Math.abs(a);
        b.push(bb(Math.floor(a / 60) % 100, 2), ":", bb(a % 60, 2));
        return b.join("")
    }

    function ps(a, b) {
        b = Date.UTC(b.getUTCFullYear(), b.getUTCMonth(), b.getUTCDate(), b.getUTCHours(), b.getUTCMinutes()) / 36E5;
        for (var c = 0; c < a.C.length && b >= a.C[c];) c += 2;
        return 0 == c ? 0 : a.C[c - 1]
    };

    function qs(a, b) {
        this.B = [];
        this.A = b || gs;
        "number" == typeof a ? rs(this, a) : ss(this, a)
    }
    var ts = [/^\'(?:[^\']|\'\')*(\'|$)/, /^(?:G+|y+|M+|k+|S+|E+|a+|h+|K+|H+|c+|L+|Q+|d+|m+|s+|v+|V+|w+|z+|Z+)/, /^[^\'GyMkSEahKHcLQdmsvVwzZ]+/];

    function us(a) {
        return a.getHours ? a.getHours() : 0
    }

    function ss(a, b) {
        for (vs && (b = b.replace(/\u200f/g, "")); b;) {
            for (var c = b, d = 0; d < ts.length; ++d) {
                var e = b.match(ts[d]);
                if (e) {
                    var f = e[0];
                    b = b.substring(f.length);
                    0 == d && ("''" == f ? f = "'" : (f = f.substring(1, "'" == e[1] ? f.length - 1 : f.length), f = f.replace(/\'\'/g, "'")));
                    a.B.push({
                        text: f,
                        type: d
                    });
                    break
                }
            }
            if (c === b) throw Error("Malformed pattern part: " + b);
        }
    }
    qs.prototype.format = function(a, b) {
        if (!a) throw Error("The date to format must be non-null.");
        var c = b ? 6E4 * (a.getTimezoneOffset() - (b.B - ps(b, a))) : 0,
            d = c ? new Date(a.getTime() + c) : a,
            e = d;
        b && d.getTimezoneOffset() != a.getTimezoneOffset() && (e = 6E4 * (d.getTimezoneOffset() - a.getTimezoneOffset()), d = new Date(d.getTime() + e), c += 0 < c ? -864E5 : 864E5, e = new Date(a.getTime() + c));
        for (var c = [], f = 0; f < this.B.length; ++f) {
            var g = this.B[f].text;
            1 == this.B[f].type ? c.push(ws(this, g, a, d, e, b)) : c.push(g)
        }
        return c.join("")
    };

    function rs(a, b) {
        var c;
        if (4 > b) c = a.A.gf[b];
        else if (8 > b) c = a.A.jf[b - 4];
        else if (12 > b) c = a.A.Sh[b - 8], c = c.replace("{1}", a.A.gf[b - 8]), c = c.replace("{0}", a.A.jf[b - 8]);
        else {
            rs(a, 10);
            return
        }
        ss(a, c)
    }

    function xs(a, b) {
        b = String(b);
        a = a.A || gs;
        if (void 0 !== a.ri) {
            for (var c = [], d = 0; d < b.length; d++) {
                var e = b.charCodeAt(d);
                c.push(48 <= e && 57 >= e ? String.fromCharCode(a.ri + e - 48) : b.charAt(d))
            }
            b = c.join("")
        }
        return b
    }
    var vs = !1;

    function ys(a) {
        if (!(a.getHours && a.getSeconds && a.getMinutes)) throw Error("The date to format has no time (probably a goog.date.Date). Use Date or goog.date.DateTime, or use a pattern without time fields.");
    }

    function ws(a, b, c, d, e, f) {
        var g = b.length;
        switch (b.charAt(0)) {
            case "G":
                return c = 0 < d.getFullYear() ? 1 : 0, 4 <= g ? a.A.Xh[c] : a.A.Yh[c];
            case "y":
                return c = d.getFullYear(), 0 > c && (c = -c), 2 == g && (c %= 100), xs(a, bb(c, g));
            case "M":
                a: switch (c = d.getMonth(), g) {
                    case 5:
                        g = a.A.bi[c];
                        break a;
                    case 4:
                        g = a.A.ai[c];
                        break a;
                    case 3:
                        g = a.A.fi[c];
                        break a;
                    default:
                        g = xs(a, bb(c + 1, g))
                }
                return g;
            case "k":
                return ys(e), xs(a, bb(us(e) || 24, g));
            case "S":
                return c = e.getTime() % 1E3 / 1E3, xs(a, c.toFixed(Math.min(3, g)).substr(2) + (3 < g ? bb(0, g - 3) : ""));
            case "E":
                return c =
                    d.getDay(), 4 <= g ? a.A.oi[c] : a.A.hi[c];
            case "a":
                return ys(e), g = us(e), a.A.Rh[12 <= g && 24 > g ? 1 : 0];
            case "h":
                return ys(e), xs(a, bb(us(e) % 12 || 12, g));
            case "K":
                return ys(e), xs(a, bb(us(e) % 12, g));
            case "H":
                return ys(e), xs(a, bb(us(e), g));
            case "c":
                a: switch (c = d.getDay(), g) {
                    case 5:
                        g = a.A.ki[c];
                        break a;
                    case 4:
                        g = a.A.ni[c];
                        break a;
                    case 3:
                        g = a.A.mi[c];
                        break a;
                    default:
                        g = xs(a, bb(c, 1))
                }
                return g;
            case "L":
                a: switch (c = d.getMonth(), g) {
                    case 5:
                        g = a.A.ji[c];
                        break a;
                    case 4:
                        g = a.A.ii[c];
                        break a;
                    case 3:
                        g = a.A.li[c];
                        break a;
                    default:
                        g = xs(a, bb(c +
                            1, g))
                }
                return g;
            case "Q":
                return c = Math.floor(d.getMonth() / 3), 4 > g ? a.A.gi[c] : a.A.di[c];
            case "d":
                return xs(a, bb(d.getDate(), g));
            case "m":
                return ys(e), xs(a, bb(e.getMinutes(), g));
            case "s":
                return ys(e), xs(a, bb(e.getSeconds(), g));
            case "v":
                return g = f || ns(c.getTimezoneOffset()), g.D;
            case "V":
                return a = f || ns(c.getTimezoneOffset()), 2 >= g ? a.D : 0 < ps(a, c) ? y(a.A.Wh) ? a.A.Wh : a.A.DST_GENERIC_LOCATION : y(a.A.hf) ? a.A.hf : a.A.STD_GENERIC_LOCATION;
            case "w":
                return c = a.A.$h, e = new Date(e.getFullYear(), e.getMonth(), e.getDate()),
                    b = a.A.Zh || 0, c = e.valueOf() + 864E5 * (((y(c) ? c : 3) - b + 7) % 7 - ((e.getDay() + 6) % 7 - b + 7) % 7), xs(a, bb(Math.floor(Math.round((c - (new Date((new Date(c)).getFullYear(), 0, 1)).valueOf()) / 864E5) / 7) + 1, g));
            case "z":
                return a = f || ns(c.getTimezoneOffset()), 4 > g ? a.F[0 < ps(a, c) ? 2 : 0] : a.F[0 < ps(a, c) ? 3 : 1];
            case "Z":
                return e = f || ns(c.getTimezoneOffset()), 4 > g ? (g = -(e.B - ps(e, c)), a = [0 > g ? "-" : "+"], g = Math.abs(g), a.push(bb(Math.floor(g / 60) % 100, 2), bb(g % 60, 2)), g = a.join("")) : g = xs(a, os(e.B - ps(e, c))), g;
            default:
                return ""
        }
    };

    function zs(a) {
        var b = new le;
        U(b, a);
        return b
    }

    function As(a, b) {
        return Bs(xe(a), xe(b)) && Bs(we(a), we(b)) && Bs(ye(a), ye(b), 1)
    }

    function Bs(a, b, c) {
        return Math.abs(a - b) < (y(c) ? c : 1E-7)
    }

    function Cs(a, b) {
        if (K(a, 0)) {
            var c = qe(a),
                d = re(b);
            K(c, 0) && ze(d, ye(c));
            K(c, 2) && (d.data[2] = xe(c));
            K(c, 1) && (d.data[1] = we(c))
        }
        K(a, 1) && (c = se(a), d = te(b), K(c, 0) && (d.data[0] = Ae(c)), K(c, 2) && (d.data[2] = N(c, 2)), K(c, 1) && (d.data[1] = N(c, 1)));
        K(a, 3) && (b.data[3] = pe(a))
    };

    function Ds(a) {
        var b = new wh;
        U(b, a);
        return b
    }

    function Es(a) {
        var b = qh(a.ia());
        if (K(b, 1)) return 2 == L(b, 1);
        a = zh(a);
        return 1 === a || 2 === a || 4 === a || 13 === a || 11 === a || 5 === a
    }

    function Fs(a) {
        var b = qh(a.ia());
        if (K(b, 1)) return 3 == L(b, 1);
        a = zh(a);
        return 3 === a || 10 === a || 15 === a || 12 === a || 7 === a || 27 === a
    }

    function Gs(a) {
        a = new Se(th(a.ia()).data[5]);
        for (var b = 0; b < S(a, 3); b++)
            if (9 == Lc(a, 3, b)) return !0;
        return !1
    }

    function Hs(a, b) {
        for (var c = 0; c < S(a, 5); c++)
            if (b(uh(a, c))) return uh(a, c);
        return null
    }

    function Is(a) {
        return Hs(a, function(a) {
            return 1 == L(new Cg(a.data[0]), 0)
        })
    }

    function Js(a, b) {
        var c = !1;
        if (K(a, 0)) {
            var d = new Te(a.data[0]);
            if (K(d, 0) && K($e(d), 2) && K($e(d), 3)) {
                var c = N($e(d), 2),
                    e = N($e(d), 3);
                re(b).data[2] = c;
                re(b).data[1] = e;
                c = !0
            }
            K(d, 2) && (d = new Xe(d.data[2]), c = te(b), K(d, 0) && (c.data[0] = N(d, 0)), K(d, 1) && (c.data[1] = N(d, 1, 90)), K(d, 2) && (c.data[2] = N(d, 2)), c = !0)
        }
        K(a, 2) && (b.data[3] = N(a, 2), c = !0);
        K(a, 1) && (a = new ld(a.data[1]), b = ve(b), Be(b, a.W()), De(b, N(a, 0)), c = !0);
        return c
    }
    var Ks = new qs("MMM yyyy");

    function Ls(a) {
        if (K(th(a), 7)) {
            a = new Le(th(a).data[7]);
            var b = ge(new de(a.data[8]));
            return b ? b : Ks.format(new ls(N(a, 0), K(a, 1) ? N(a, 1) - 1 : void 0, K(a, 2) ? N(a, 2) : void 0, K(a, 3) ? N(a, 3) : void 0, K(a, 4) ? N(a, 4) : void 0))
        }
        return ""
    }

    function Ms(a, b) {
        return K(a, 21) && K(b, 21) ? (a = a.ia(), b = b.ia(), K(a, 1) && K(b, 1) && L(ph(a), 0) == L(ph(b), 0) && ph(a).za() == ph(b).za()) : K(a, 0) && K(b, 0) ? a.za() == b.za() : !1
    }

    function Ns(a) {
        return K(a.ia(), 1) ? ph(a.ia()).za() : K(a, 0) ? a.za() : ""
    };

    function Os(a) {
        this.data = a || []
    }
    H(Os, J);

    function Ps(a) {
        this.data = a || []
    }
    H(Ps, J);

    function Qs(a) {
        this.data = a || []
    }
    H(Qs, J);
    Os.prototype.fa = function() {
        return new le(this.data[8])
    };

    function Rs(a) {
        return new le(P(a, 8))
    }
    Qs.prototype.eb = function() {
        return new wh(this.data[4])
    };

    function Ss(a) {
        this.data = a || []
    }
    H(Ss, J);

    function Ts(a) {
        this.data = a || []
    }
    H(Ts, J);

    function Us(a) {
        this.data = a || []
    }
    H(Us, J);
    Ts.prototype.za = function() {
        return new Us(this.data[0])
    };

    function Vs(a) {
        this.data = a || []
    }
    H(Vs, J);

    function Ws(a) {
        this.data = a || []
    }
    H(Ws, J);

    function Xs(a) {
        return L(a, 0)
    }

    function Ys(a) {
        return new wh(a.data[4])
    }

    function Zs(a) {
        return new wh(P(a, 4))
    }

    function $s(a) {
        return new Ts(a.data[8])
    };

    function at(a) {
        this.data = a || []
    }
    H(at, J);

    function bt(a) {
        this.data = a || []
    }
    H(bt, J);

    function ct(a) {
        this.data = a || []
    }
    H(ct, J);
    ct.prototype.fa = function() {
        return new le(this.data[0])
    };

    function dt(a) {
        var b = new Os,
            c = null,
            d = null,
            e = null;
        b.data[13] = K(sh(a), 5) ? ge(new de(sh(a).data[5])) : ge(new de(sh(a).data[6]));
        for (var f = 0; f < S(a, 5); ++f) {
            var g = uh(a, f);
            K(g, 1) && (e = e || g, 2 === L(new Cg(g.data[0]), 0) && (c = g), 1 === L(new Cg(g.data[0]), 0) && (d = g))
        }
        null == e && 0 < S(a, 5) && (e = uh(a, 0));
        d && (et(d, b), f = Tg(d), (new me(P(b, 9))).data[2] = N($e(f), 2), (new me(P(b, 9))).data[1] = N($e(f), 3));
        c && et(c, b);
        c || d || !e || et(e, b);
        if (K(a, 4)) {
            d = new Ie(a.data[4]);
            c = [];
            for (f = 0; f < S(d, 0); ++f) e = new Je(Nc(d, 0, f)), c.push(ge(Ke(e)));
            for (f =
                0; f < S(d, 1); ++f) e = new Je(Nc(d, 1, f)), c.push(ge(Ke(e)));
            for (f = 0; f < S(d, 2); ++f) e = new Je(Nc(d, 2, f)), c.push(ge(Ke(e)));
            0 < S(d, 4) && (f = new Je(Nc(d, 4, 0)), K(f, 1) && (b.data[14] = O(f, 1)));
            b.data[11] = c.join(" ")
        }
        K(a, 1) && (f = ph(a).za(), b.data[0] = f);
        K(a, 2) && (f = qh(a), c = ft(L(f, 0), L(f, 1), th(a)), b.data[6] = c, Be(new oe(P(b, 3)), (new ld(jf(f).data[1])).W()), De(new oe(P(b, 3)), N(new ld(jf(f).data[1]), 0)), b.data[4] = S(jf(f), 0) - 1, b.data[5] = S(jf(f), 0) - 1, K(f, 2) && (Be(ve(Rs(b)), (new ld(f.data[2])).W()), De(ve(Rs(b)), N(new ld(f.data[2]),
            0))));
        if ((0 == (new oe(b.data[3])).W() || 0 == Ce(new oe(b.data[3]))) && 3 == L(ph(a), 0)) {
            Be(new oe(P(b, 3)), 512);
            De(new oe(P(b, 3)), 512);
            f = ue(b.fa());
            f = Math.max(f.W(), Ce(f));
            c = 0;
            for (d = 512; d < f;) d <<= 1, c++;
            f = c;
            b.data[4] = f;
            b.data[5] = f
        }
        b.data[1] = 1 != L(new Dd(a.data[7]), 1, 1);
        return b
    }

    function ft(a, b, c) {
        switch (a) {
            case 2:
                switch (b) {
                    case 2:
                        if (2 == L(c, 0)) return 11;
                        if (1 == L(c, 0)) return 2;
                        for (a = 0; a < S(c, 3); ++a)
                            if (1 == Lc(c, 3, a)) return 2;
                        return 3 == L(c, 0) ? 1 : 11;
                    case 1:
                        return 24;
                    default:
                        return 3
                }
            case 3:
                switch (b) {
                    case 2:
                        return 11;
                    default:
                        return 12
                }
            case 4:
                return 10;
            case 5:
                return 18
        }
        return 27
    }

    function gt(a) {
        switch (a) {
            case 1:
                return 7;
            case 2:
                return 0;
            case 3:
                return 4;
            case 4:
                return 1;
            default:
                return 0
        }
    }

    function et(a, b) {
        var c = Rs(b),
            d = Tg(a),
            e = new Bf;
        U(new Te(P(e, 0)), d);
        Js(e, c);
        K(se(c), 1) || (te(c).data[1] = 90);
        K(qe(c), 0) || ze(re(c), 3);
        K(new Xe(d.data[2]), 0) && (b.data[10] = N(new Xe(d.data[2]), 0));
        K(d, 3) && (b.data[2] = O(new Ye(d.data[3]), 0, ""));
        for (c = 0; c < S(a, 12); ++c) {
            var f = new Ag(Nc(a, 12, c));
            if (S(f, 0))
                for (var e = new gg(Nc(f, 0, 0)), g = ge(new de(e.data[2])), k = 0; k < S(f, 1); ++k) {
                    e = new Qs(Mc(b, 19));
                    (new wh(P(e, 4))).data[1] = 0;
                    (new wh(P(e, 4))).data[2] = 1;
                    e.data[3] = g;
                    var l = Lc(f, 1, k);
                    e.data[0] = l
                }
        }
        for (c = 0; c < S(a, 6); ++c)
            if (e =
                new Hg(Nc(a, 6, c)), K(e, 0) && (k = Vg(new Kg(a.data[3]), N(e, 0)), K(k, 2))) {
                var e = new Qs(Mc(b, 19)),
                    f = N($e(new Te(k.data[2])), 2),
                    g = N($e(new Te(k.data[2])), 3),
                    l = Jb(N($e(d), 2)),
                    m = Jb(f),
                    n = Jb(g) - Jb(N($e(d), 3));
                n > Math.PI ? n -= 2 * Math.PI : n < -Math.PI && (n += 2 * Math.PI);
                sl(em, 0, 0, 1);
                sl(fm, Math.cos(l), 0, Math.sin(l));
                sl(gm, Math.cos(n) * Math.cos(m), Math.sin(n) * Math.cos(m), Math.sin(m));
                Al(gm, fm, hm);
                e.data[0] = (360 + Kb(Math.atan2((0 < n ? 1 : 0 > n ? -1 : n) * Math.sqrt(hm[0] * hm[0] + hm[2] * hm[2]), hm[1]))) % 360;
                e = new wh(P(e, 4));
                l = gt(L(Ug(k), 0));
                e.data[1] =
                    l;
                e.data[2] = 1;
                k = Ug(k).za();
                e.data[0] = k;
                e = re(Bh(e));
                e.data[2] = f;
                e.data[1] = g;
                ze(e, 3)
            }
        for (c = 0; c < S(a, 7); ++c) d = N(new Ig(Nc(a, 7, c)), 0), 0 > d || d >= S(new Kg(a.data[3]), 0) || (f = Vg(new Kg(a.data[3]), d), d = new Ye((new Te(f.data[2])).data[3]), e = new Ps(Mc(b, 20)), f = Ug(f).za(), e.data[1] = f, e.data[0] = O(d, 0, ""), e.data[2] = N(d, 1), K(d, 3) && (e.data[4] = ge(new de(d.data[3]))), K(d, 2) && (e.data[3] = ge(new de(d.data[2]))))
    };

    function ht(a) {
        this.A = a
    }
    ht.prototype.Ha = function(a, b) {
        a = new es;
        a.A = Sr.Pg(this.A);
        if (a.A) {
            a.C = dt(a.A);
            var c = new ds("");
            a.B.push(c);
            b(a)
        } else b(null)
    };

    function it(a) {
        V.call(this, "CUCS", wb(arguments))
    }
    H(it, V);

    function jt(a, b, c) {
        a(new ht(c))
    };

    function kt() {}
    kt.prototype.A = function(a) {
        var b;
        a: {
            if (a = a.A) {
                var c = [];
                for (b in a) kr(b, a[b], c);
                c[0] = "";
                b = c.join("");
                if ("" != b) break a
            }
            b = ""
        }
        return b
    };

    function lt(a) {
        this.A = {};
        this.ta = a || ""
    }
    lt.prototype.za = h("ta");

    function mt(a, b) {
        this.A = [];
        for (var c = 0; c < b.length; c++) this.A.push(new lr(b[c]));
        this.B = a;
        this.C = new kt
    }
    mt.prototype.tb = function(a, b, c, d, e, f, g) {
        var k = new lt;
        a = Ns(Ys(a.Da()));
        if (!a) return C;
        k.A.panoid = a;
        k.A.output = "tile";
        k.A.x = "" + b;
        k.A.y = "" + c;
        k.A.zoom = "" + d;
        k.A.nbt = "";
        k.A.fover = "2";
        b = this.A[(b + c) % this.A.length];
        e = f.sa(e, "cts-get-tile");
        return this.B.Ja(nt(this, b, k), e, g)
    };

    function nt(a, b, c) {
        b = b.toString();
        a = a.C.A(c);
        return -1 == b.indexOf("?") ? b + "?" + a : b + "&" + a
    }
    mt.prototype.Tb = C;

    function ot(a, b) {
        V.call(this, "CTS", wb(arguments))
    }
    H(ot, V);

    function pt(a, b, c, d) {
        b = new mt(c, d);
        a(b)
    };

    function qt(a, b, c, d) {
        V.call(this, "FPSC", wb(arguments))
    }
    H(qt, V);

    function rt(a, b, c) {
        V.call(this, "FPCS", wb(arguments))
    }
    H(rt, V);

    function st(a, b, c, d) {
        V.call(this, "FPTS", wb(arguments))
    }
    H(st, V);

    function tt(a, b, c) {
        this.C = c;
        this.B = a;
        this.A = b
    }
    tt.prototype.Ac = function(a, b, c) {
        var d = this;
        yj([this.A, this.B], C, b);
        this.B.get(function(b, f) {
            b.Ha(a, function(b) {
                c && f.ua(c);
                ut(d, a, f, b)
            }, f)
        }, b)
    };
    tt.prototype.Gb = function(a, b, c, d, e, f) {
        function g(g, k) {
            f && e.ua(f);
            g ? k ? a.Hd(new fn(b, c, d, g), e) : a.Gd(new fn(b, c, d, g), e) : a.Le() ? a.Dd(b, c, d) : a.Fc(b, c, d)
        }
        var k = null,
            l = !1;
        yj([this.A, this.B], C, e);
        this.A.get(function(e, f) {
            l || (k = e.tb(a, b, c, d, g, f))
        }, e);
        return function() {
            l = !0;
            k && k()
        }
    };

    function ut(a, b, c, d) {
        if (d && d.C) {
            d.A && b.je(d.A);
            var e = d.C;
            a.A.get(function(a, b) {
                a.Tb(e, b)
            }, c);
            b.Tb(e, c);
            for (c = 0; c < d.B.length; c++) {
                var f = d.B[c];
                f.$d(b);
                jq(a.C, f, kq(f.Zd(), !1))
            }
        } else b.he()
    };

    function vt(a, b) {
        null != a && this.Vd.apply(this, arguments)
    }
    q = vt.prototype;
    q.Xb = "";
    q.set = function(a) {
        this.Xb = "" + a
    };
    q.Vd = function(a, b, c) {
        this.Xb += String(a);
        if (null != b)
            for (var d = 1; d < arguments.length; d++) this.Xb += arguments[d];
        return this
    };
    q.clear = function() {
        this.Xb = ""
    };
    q.toString = h("Xb");

    function wt(a, b) {
        var c = Array.prototype.slice.call(arguments),
            d = c.shift();
        if ("undefined" == typeof d) throw Error("[goog.string.format] Template required");
        d.replace(/%([0\-\ \+]*)(\d+)?(\.(\d+))?([%sfdiu])/g, function(a, b, d, k, l, m, n, p) {
            if ("%" == m) return "%";
            var e = c.shift();
            if ("undefined" == typeof e) throw Error("[goog.string.format] Not enough arguments");
            arguments[0] = e;
            return xt[m].apply(null, arguments)
        })
    }
    var xt = {
        s: function(a, b, c) {
            return isNaN(c) || "" == c || a.length >= Number(c) ? a : a = -1 < b.indexOf("-", 0) ? a + ab(" ", Number(c) - a.length) : ab(" ", Number(c) - a.length) + a
        },
        f: function(a, b, c, d, e) {
            d = a.toString();
            isNaN(e) || "" == e || (d = parseFloat(a).toFixed(e));
            var f;
            f = 0 > Number(a) ? "-" : 0 <= b.indexOf("+") ? "+" : 0 <= b.indexOf(" ") ? " " : "";
            0 <= Number(a) && (d = f + d);
            if (isNaN(c) || d.length >= Number(c)) return d;
            d = isNaN(e) ? Math.abs(Number(a)).toString() : Math.abs(Number(a)).toFixed(e);
            a = Number(c) - d.length - f.length;
            return d = 0 <= b.indexOf("-", 0) ?
                f + d + ab(" ", a) : f + ab(0 <= b.indexOf("0", 0) ? "0" : " ", a) + d
        },
        d: function(a, b, c, d, e, f, g, k) {
            return xt.f(parseInt(a, 10), b, c, d, 0, f, g, k)
        }
    };
    xt.i = xt.d;
    xt.u = xt.d;

    function yt(a) {
        this.A = void 0;
        this.Ia = {};
        if (a) {
            var b = dj(a);
            a = cj(a);
            for (var c = 0; c < b.length; c++) this.set(b[c], a[c])
        }
    }
    q = yt.prototype;
    q.set = function(a, b) {
        zt(this, a, b, !1)
    };
    q.add = function(a, b) {
        zt(this, a, b, !0)
    };

    function zt(a, b, c, d) {
        for (var e = 0; e < b.length; e++) {
            var f = b.charAt(e);
            a.Ia[f] || (a.Ia[f] = new yt);
            a = a.Ia[f]
        }
        if (d && void 0 !== a.A) throw Error('The collection already contains the key "' + b + '"');
        a.A = c
    }

    function At(a, b) {
        for (var c = 0; c < b.length; c++)
            if (a = a.Ia[b.charAt(c)], !a) return;
        return a
    }
    q.get = function(a) {
        return (a = At(this, a)) ? a.A : void 0
    };
    q.Ca = function() {
        var a = [];
        Xt(this, a);
        return a
    };

    function Xt(a, b) {
        void 0 !== a.A && b.push(a.A);
        for (var c in a.Ia) Xt(a.Ia[c], b)
    }
    q.kb = function(a) {
        var b = [];
        if (a) {
            for (var c = this, d = 0; d < a.length; d++) {
                var e = a.charAt(d);
                if (!c.Ia[e]) return [];
                c = c.Ia[e]
            }
            Yt(c, a, b)
        } else Yt(this, "", b);
        return b
    };

    function Yt(a, b, c) {
        void 0 !== a.A && c.push(b);
        for (var d in a.Ia) Yt(a.Ia[d], b + d, c)
    }
    q.clear = function() {
        this.Ia = {};
        this.A = void 0
    };
    q.remove = function(a) {
        for (var b = this, c = [], d = 0; d < a.length; d++) {
            var e = a.charAt(d);
            if (!b.Ia[e]) throw Error('The collection does not have the key "' + a + '"');
            c.push([b, e]);
            b = b.Ia[e]
        }
        a = b.A;
        for (delete b.A; 0 < c.length;)
            if (e = c.pop(), b = e[0], e = e[1], b.Ia[e].Oa()) delete b.Ia[e];
            else break;
        return a
    };
    q.Bb = function() {
        var a = this.Ca();
        if (a.Bb && "function" == typeof a.Bb) a = a.Bb();
        else if (va(a) || wa(a)) a = a.length;
        else {
            var b = 0,
                c;
            for (c in a) b++;
            a = b
        }
        return a
    };
    q.Oa = function() {
        return void 0 === this.A && Qb(this.Ia)
    };

    function Zt(a) {
        return /^\s*$/.test(a) ? !1 : /^[\],:{}\s\u2028\u2029]*$/.test(a.replace(/\\["\\\/bfnrtu]/g, "@").replace(/(?:"[^"\\\n\r\u2028\u2029\x00-\x08\x0a-\x1f]*"|true|false|null|-?\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?)[\s\u2028\u2029]*(?=:|,|]|}|$)/g, "]").replace(/(?:^|:|,)(?:[\s\u2028\u2029]*\[)+/g, ""))
    }

    function $t(a) {
        a = String(a);
        if (Zt(a)) try {
            return eval("(" + a + ")")
        } catch (b) {}
        throw Error("Invalid JSON string: " + a);
    };

    function au() {}
    var bu = "function" == typeof Uint8Array;

    function cu(a, b) {
        a.A = null;
        b || (b = []);
        a.N = void 0;
        a.D = -1;
        a.B = b;
        a: {
            if (a.B.length) {
                b = a.B.length - 1;
                var c = a.B[b];
                if (c && "object" == typeof c && !ua(c) && !(bu && c instanceof Uint8Array)) {
                    a.F = b - a.D;
                    a.C = c;
                    break a
                }
            }
            a.F = Number.MAX_VALUE
        }
        a.L = {}
    }
    var du = [];

    function W(a, b) {
        if (b < a.F) {
            b += a.D;
            var c = a.B[b];
            return c === du ? a.B[b] = [] : c
        }
        c = a.C[b];
        return c === du ? a.C[b] = [] : c
    }

    function eu(a, b) {
        a = W(a, b);
        return null == a ? a : +a
    }

    function fu(a, b) {
        a = W(a, b);
        return null == a ? !1 : a
    }

    function X(a, b, c) {
        b < a.F ? a.B[b + a.D] = c : a.C[b] = c
    }

    function gu(a) {
        if (a.A)
            for (var b in a.A) {
                var c = a.A[b];
                if (ua(c))
                    for (var d = 0; d < c.length; d++) c[d] && c[d].ib();
                else c && c.ib()
            }
    }
    au.prototype.ib = function() {
        gu(this);
        return this.B
    };
    au.prototype.toString = function() {
        gu(this);
        return this.B.toString()
    };
    au.prototype.getExtension = function(a) {
        if (this.C) {
            this.A || (this.A = {});
            var b = a.C;
            if (a.D) {
                if (a.B()) return this.A[b] || (this.A[b] = ob(this.C[b] || [], function(b) {
                    return new a.A(b)
                })), this.A[b]
            } else if (a.B()) return !this.A[b] && this.C[b] && (this.A[b] = new a.A(this.C[b])), this.A[b];
            return this.C[b]
        }
    };

    function hu(a) {
        cu(this, a)
    }
    H(hu, au);
    q = hu.prototype;
    q.ee = function() {
        return W(this, 1)
    };
    q.dh = function(a) {
        X(this, 1, a)
    };
    q.W = function() {
        return W(this, 12)
    };
    q.lh = function(a) {
        X(this, 12, a)
    };
    q.We = function() {
        return W(this, 13)
    };
    q.Zg = function(a) {
        X(this, 13, a)
    };
    q.Jj = function() {
        return W(this, 33)
    };
    q.wm = function(a) {
        X(this, 33, a)
    };
    q.dg = function() {
        return fu(this, 2)
    };
    q.Yg = function(a) {
        X(this, 2, a)
    };
    q.bg = function() {
        return W(this, 51)
    };
    q.Xg = function(a) {
        X(this, 51, a)
    };
    q.eg = function() {
        return W(this, 32)
    };
    q.ah = function(a) {
        X(this, 32, a)
    };
    q.kg = function() {
        return fu(this, 19)
    };
    q.eh = function(a) {
        X(this, 19, a)
    };
    q.lg = function() {
        return fu(this, 52)
    };
    q.gh = function(a) {
        X(this, 52, a)
    };
    q.mg = function() {
        return fu(this, 67)
    };
    q.hh = function(a) {
        X(this, 67, a)
    };
    q.Si = function() {
        return W(this, 80)
    };
    q.Al = function(a) {
        X(this, 80, a)
    };
    q.ag = function() {
        return fu(this, 20)
    };
    q.Wg = function(a) {
        X(this, 20, a)
    };
    q.jg = function() {
        return W(this, 60)
    };
    q.bh = function(a) {
        X(this, 60, a)
    };
    q.Ti = function() {
        return fu(this, 3)
    };
    q.Bl = function(a) {
        X(this, 3, a)
    };
    q.nl = function() {
        return fu(this, 4)
    };
    q.Ll = function(a) {
        X(this, 4, a)
    };
    q.hj = function() {
        return W(this, 65)
    };
    q.Tl = function(a) {
        X(this, 65, a)
    };
    q.ng = function() {
        return W(this, 9)
    };
    q.ih = function(a) {
        X(this, 9, a)
    };
    q.og = function() {
        return W(this, 10)
    };
    q.jh = function(a) {
        X(this, 10, a)
    };
    q.Vg = function() {
        return W(this, 11)
    };
    q.kh = function(a) {
        X(this, 11, a)
    };
    q.Nj = function() {
        return fu(this, 14)
    };
    q.zm = function(a) {
        X(this, 14, a)
    };
    q.Zi = function() {
        return fu(this, 34)
    };
    q.Hl = function(a) {
        X(this, 34, a)
    };
    q.Oj = function() {
        return fu(this, 72)
    };
    q.Am = function(a) {
        X(this, 72, a)
    };
    q.ml = function() {
        return W(this, 15)
    };
    q.Dl = function(a) {
        X(this, 15, a)
    };
    q.dj = function() {
        return W(this, 16)
    };
    q.Nl = function(a) {
        X(this, 16, a)
    };
    q.gj = function() {
        return W(this, 17)
    };
    q.Sl = function(a) {
        X(this, 17, a)
    };
    q.qg = function() {
        return W(this, 18)
    };
    q.Cm = function(a) {
        X(this, 18, a)
    };
    q.tg = function() {
        return W(this, 45)
    };
    q.Dm = function(a) {
        X(this, 45, a)
    };
    q.pl = function() {
        return W(this, 22)
    };
    q.Pl = function(a) {
        X(this, 22, a)
    };
    q.ej = function() {
        return W(this, 54)
    };
    q.Ql = function(a) {
        X(this, 54, a)
    };
    q.Kj = function() {
        return W(this, 82)
    };
    q.xm = function(a) {
        X(this, 82, a)
    };
    q.uj = function() {
        return W(this, 83)
    };
    q.gm = function(a) {
        X(this, 83, a)
    };
    q.Ui = function() {
        return W(this, 21)
    };
    q.Cl = function(a) {
        X(this, 21, a)
    };
    q.Ri = function() {
        return fu(this, 23)
    };
    q.zl = function(a) {
        X(this, 23, a)
    };
    q.rl = function() {
        return W(this, 24)
    };
    q.Bm = function(a) {
        X(this, 24, a)
    };
    q.Pj = function() {
        return W(this, 36)
    };
    q.Em = function(a) {
        X(this, 36, a)
    };
    q.Ij = function() {
        return fu(this, 6)
    };
    q.vm = function(a) {
        X(this, 6, a)
    };
    q.Gj = function() {
        return W(this, 26)
    };
    q.tm = function(a) {
        X(this, 26, a)
    };
    q.bj = function() {
        return W(this, 30)
    };
    q.Kl = function(a) {
        X(this, 30, a)
    };
    q.Qj = function() {
        return W(this, 31)
    };
    q.Fm = function(a) {
        X(this, 31, a)
    };
    q.ql = function() {
        return W(this, 27)
    };
    q.bm = function(a) {
        X(this, 27, a)
    };
    q.xj = function() {
        return W(this, 28)
    };
    q.jm = function(a) {
        X(this, 28, a)
    };
    q.Bj = function() {
        return W(this, 57)
    };
    q.om = function(a) {
        X(this, 57, a)
    };
    q.Cj = function() {
        return W(this, 58)
    };
    q.pm = function(a) {
        X(this, 58, a)
    };
    q.zj = function() {
        return W(this, 59)
    };
    q.lm = function(a) {
        X(this, 59, a)
    };
    q.Dj = function() {
        return fu(this, 35)
    };
    q.qm = function(a) {
        X(this, 35, a)
    };
    q.Ej = function() {
        return fu(this, 41)
    };
    q.rm = function(a) {
        X(this, 41, a)
    };
    q.yj = function() {
        return fu(this, 64)
    };
    q.km = function(a) {
        X(this, 64, a)
    };
    q.pj = function() {
        return fu(this, 48)
    };
    q.am = function(a) {
        X(this, 48, a)
    };
    q.Aj = function() {
        return fu(this, 49)
    };
    q.nm = function(a) {
        X(this, 49, a)
    };
    q.mj = function() {
        return fu(this, 37)
    };
    q.Yl = function(a) {
        X(this, 37, a)
    };
    q.Qi = function() {
        return W(this, 38)
    };
    q.yl = function(a) {
        X(this, 38, a)
    };
    q.Pi = function() {
        return W(this, 86)
    };
    q.xl = function(a) {
        X(this, 86, a)
    };
    q.ll = function() {
        return W(this, 39)
    };
    q.wl = function(a) {
        X(this, 39, a)
    };
    q.kl = function() {
        return W(this, 87)
    };
    q.tl = function(a) {
        X(this, 87, a)
    };
    q.qj = function() {
        return W(this, 88)
    };
    q.cm = function(a) {
        X(this, 88, a)
    };
    q.Mj = function() {
        return W(this, 89)
    };
    q.ym = function(a) {
        X(this, 89, a)
    };
    q.nj = function() {
        return W(this, 40)
    };
    q.Zl = function(a) {
        X(this, 40, a)
    };
    q.Xi = function() {
        return W(this, 42)
    };
    q.Fl = function(a) {
        X(this, 42, a)
    };
    q.Wi = function() {
        return W(this, 43)
    };
    q.El = function(a) {
        X(this, 43, a)
    };
    q.wj = function() {
        return W(this, 44)
    };
    q.im = function(a) {
        X(this, 44, a)
    };
    q.vj = function() {
        return W(this, 62)
    };
    q.hm = function(a) {
        X(this, 62, a)
    };
    q.oj = function() {
        return W(this, 46)
    };
    q.$l = function(a) {
        X(this, 46, a)
    };
    q.tj = function() {
        return W(this, 61)
    };
    q.em = function(a) {
        X(this, 61, a)
    };
    q.$i = function() {
        return W(this, 50)
    };
    q.Il = function(a) {
        X(this, 50, a)
    };
    q.lj = function() {
        return W(this, 53)
    };
    q.Xl = function(a) {
        X(this, 53, a)
    };
    q.kj = function() {
        return W(this, 55)
    };
    q.Wl = function(a) {
        X(this, 55, a)
    };
    q.Hj = function() {
        return W(this, 56)
    };
    q.um = function(a) {
        X(this, 56, a)
    };
    q.Sj = function() {
        return W(this, 63)
    };
    q.Hm = function(a) {
        X(this, 63, a)
    };
    q.Uj = function() {
        return W(this, 81)
    };
    q.Jm = function(a) {
        X(this, 81, a)
    };
    q.Rj = function() {
        return W(this, 68)
    };
    q.Gm = function(a) {
        X(this, 68, a)
    };
    q.Tj = function() {
        return W(this, 69)
    };
    q.Im = function(a) {
        X(this, 69, a)
    };
    q.ij = function() {
        return W(this, 66)
    };
    q.Ul = function(a) {
        X(this, 66, a)
    };
    q.cj = function() {
        return W(this, 70)
    };
    q.Ml = function(a) {
        X(this, 70, a)
    };
    q.fj = function() {
        return W(this, 71)
    };
    q.Rl = function(a) {
        X(this, 71, a)
    };
    q.jj = function() {
        return W(this, 73)
    };
    q.Vl = function(a) {
        X(this, 73, a)
    };
    q.Yi = function() {
        return W(this, 84)
    };
    q.Gl = function(a) {
        X(this, 84, a)
    };
    q.Ni = function() {
        return W(this, 74)
    };
    q.sl = function(a) {
        X(this, 74, a)
    };
    q.ol = function() {
        return W(this, 75)
    };
    q.Ol = function(a) {
        X(this, 75, a)
    };
    q.sj = function() {
        return eu(this, 76)
    };
    q.dm = function(a) {
        X(this, 76, a)
    };
    q.Vj = function() {
        return eu(this, 77)
    };
    q.Km = function(a) {
        X(this, 77, a)
    };
    q.Fj = function() {
        return eu(this, 78)
    };
    q.sm = function(a) {
        X(this, 78, a)
    };
    q.aj = function() {
        return eu(this, 79)
    };
    q.Jl = function(a) {
        X(this, 79, a)
    };
    q.Oi = function() {
        return W(this, 85)
    };
    q.ul = function(a) {
        X(this, 85, a)
    };

    function Y() {
        cu(this, void 0)
    }
    H(Y, hu);
    Y.prototype.J = ba("G");
    Y.prototype.I = h("G");
    Y.prototype.M = ba("H");
    Y.prototype.K = h("H");

    function iu() {
        if (!ju) {
            var a = ju = new yt,
                b;
            for (b in ku) a.add(b, ku[b])
        }
    }
    var ju;

    function Z(a, b) {
        this.A = a;
        this.B = b
    }
    var ku = {
        a: new Z([3, 0], [Y.prototype.Cl, Y.prototype.um]),
        al: new Z([3], [Y.prototype.sl]),
        b: new Z([3, 0], [Y.prototype.zl, Y.prototype.yl]),
        ba: new Z([0], [Y.prototype.ul]),
        bc: new Z([0], [Y.prototype.tl]),
        br: new Z([0], [Y.prototype.xl]),
        c: new Z([3, 0], [Y.prototype.Yg, Y.prototype.wl]),
        cc: new Z([3], [Y.prototype.Xg]),
        ci: new Z([3], [Y.prototype.ah]),
        d: new Z([3], [Y.prototype.Bl]),
        df: new Z([3], [Y.prototype.Al]),
        e: new Z([0], [Y.prototype.Dl]),
        f: new Z([4], [Y.prototype.Nl]),
        fg: new Z([3], [Y.prototype.Hl]),
        fh: new Z([3], [Y.prototype.Kl]),
        fm: new Z([3], [Y.prototype.Gl]),
        fo: new Z([2], [Y.prototype.Jl]),
        ft: new Z([3], [Y.prototype.Il]),
        fv: new Z([3], [Y.prototype.Fm]),
        g: new Z([3], [Y.prototype.zm]),
        gd: new Z([3], [Y.prototype.gm]),
        h: new Z([3, 0], [Y.prototype.Ll, Y.prototype.Zg]),
        i: new Z([3], [Y.prototype.Pl]),
        ic: new Z([0], [Y.prototype.Rl]),
        id: new Z([3], [Y.prototype.Ml]),
        ip: new Z([3], [Y.prototype.Ql]),
        iv: new Z([0], [Y.prototype.Ol]),
        j: new Z([1], [Y.prototype.J]),
        k: new Z([3, 0], [Y.prototype.Sl, Y.prototype.Fl]),
        l: new Z([0], [Y.prototype.im]),
        lf: new Z([3], [Y.prototype.Tl]),
        m: new Z([0], [Y.prototype.Hm]),
        mm: new Z([4], [Y.prototype.Jm]),
        mo: new Z([3], [Y.prototype.Vl]),
        mv: new Z([3], [Y.prototype.Ul]),
        n: new Z([3], [Y.prototype.Wg]),
        nc: new Z([3], [Y.prototype.Wl]),
        nd: new Z([3], [Y.prototype.Xl]),
        no: new Z([3], [Y.prototype.Yl]),
        ns: new Z([3], [Y.prototype.Zl]),
        nt0: new Z([4], [Y.prototype.Em]),
        nu: new Z([3], [Y.prototype.$l]),
        nw: new Z([3], [Y.prototype.am]),
        o: new Z([1, 3], [Y.prototype.M, Y.prototype.bm]),
        p: new Z([3, 0], [Y.prototype.eh, Y.prototype.El]),
        pa: new Z([3], [Y.prototype.em]),
        pc: new Z([0], [Y.prototype.cm]),
        pd: new Z([3], [Y.prototype.bh]),
        pf: new Z([3], [Y.prototype.hh]),
        pg: new Z([3], [Y.prototype.Am]),
        pi: new Z([2], [Y.prototype.dm]),
        pp: new Z([3], [Y.prototype.gh]),
        q: new Z([4], [Y.prototype.jm]),
        r: new Z([3, 0], [Y.prototype.vm, Y.prototype.tm]),
        rg: new Z([3], [Y.prototype.lm]),
        rh: new Z([3], [Y.prototype.nm]),
        rj: new Z([3], [Y.prototype.om]),
        ro: new Z([2], [Y.prototype.sm]),
        rp: new Z([3], [Y.prototype.pm]),
        rw: new Z([3], [Y.prototype.qm]),
        rwa: new Z([3], [Y.prototype.km]),
        rwu: new Z([3], [Y.prototype.rm]),
        s: new Z([3, 0], [Y.prototype.wm, Y.prototype.dh]),
        sc: new Z([0], [Y.prototype.ym]),
        sg: new Z([3], [Y.prototype.xm]),
        t: new Z([4], [Y.prototype.Bm]),
        u: new Z([3], [Y.prototype.Cm]),
        ut: new Z([3], [Y.prototype.Dm]),
        v: new Z([0], [Y.prototype.hm]),
        vb: new Z([0], [Y.prototype.Gm]),
        vl: new Z([0], [Y.prototype.Im]),
        w: new Z([0], [Y.prototype.lh]),
        x: new Z([0], [Y.prototype.ih]),
        y: new Z([0], [Y.prototype.jh]),
        ya: new Z([2], [Y.prototype.Km]),
        z: new Z([0], [Y.prototype.kh])
    };

    function lu(a, b) {
        wt("For token '%s': %s", a, b)
    }

    function mu(a, b) {
        var c = new Y,
            d = new Y;
        if ("" != b) {
            b = b.split("-");
            for (var e = 0; e < b.length; e++) {
                var f = b[e];
                if (0 != f.length) {
                    var g;
                    var k = f,
                        l = !1,
                        m = k;
                    g = k.substring(0, 1);
                    g != g.toLowerCase() && (l = !0, m = k.substring(0, 1).toLowerCase() + k.substring(1));
                    var n = ju;
                    for (g = 1; g <= m.length; ++g) {
                        var p = n,
                            r = m.substring(0, g);
                        if (0 == r.length ? p.Oa() : !At(p, r)) break
                    }
                    g = 1 == g ? null : (m = n.get(m.substring(0, g - 1))) ? {
                        bl: k.substring(0, g - 1),
                        value: k.substring(g - 1),
                        Om: l,
                        attributes: m
                    } : null;
                    if (g) {
                        k = [];
                        l = [];
                        m = !1;
                        for (n = 0; n < g.attributes.A.length; n++) {
                            var p =
                                g.attributes.A[n],
                                u = g.value,
                                r = e;
                            if (g.Om && 1 == p)
                                for (var t = u.length; 12 > t && r < b.length - 1;) u += "-" + b[r + 1], t = u.length, ++r;
                            else if (2 == p)
                                for (; r < b.length - 1 && b[r + 1].match(/^[\d\.]/);) u += "-" + b[r + 1], ++r;
                            t = g.attributes.B[n];
                            u = nu(a, p)(g.bl, u, c, d, t);
                            if (null === u) {
                                m = !0;
                                e = r;
                                break
                            } else k.push(p), l.push(u)
                        }
                        if (!m)
                            for (g = 0; g < l.length; g++) m = k[g], u = l[g], ou(a, m)(f, u)
                    }
                }
            }
        }
        return new pu(c, d)
    }

    function qu(a, b, c, d, e) {
        e.apply(c, [b]);
        a = a.substring(0, 1);
        e.apply(d, [a == a.toUpperCase()])
    }
    q = iu.prototype;
    q.fl = function(a, b, c, d, e) {
        if ("" == b) return 0;
        isFinite(b) && (b = String(b));
        b = wa(b) ? /^\s*-?0x/i.test(b) ? parseInt(b, 16) : parseInt(b, 10) : NaN;
        if (isNaN(b)) return 1;
        qu(a, b, c, d, e);
        return null
    };
    q.lk = function(a, b) {
        switch (b) {
            case 1:
                lu(a, "Option value could not be interpreted as an integer.");
                break;
            case 0:
                lu(a, "Missing value for integer option.")
        }
    };
    q.el = function(a, b, c, d, e) {
        if ("" == b) return 0;
        var f = Number(b);
        b = 0 == f && /^[\s\xa0]*$/.test(b) ? NaN : f;
        if (isNaN(b)) return 1;
        qu(a, b, c, d, e);
        return null
    };
    q.kk = function(a, b) {
        switch (b) {
            case 1:
                lu(a, "Option value could not be interpreted as a float.");
                break;
            case 0:
                lu(a, "Missing value for float option.")
        }
    };
    q.dl = function(a, b, c, d, e) {
        if ("" != b) return 2;
        qu(a, !0, c, d, e);
        return null
    };
    q.jk = function(a, b) {
        switch (b) {
            case 2:
                lu(a, "Unexpected value specified for boolean option.")
        }
    };
    q.gl = function(a, b, c, d, e) {
        if ("" == b) return 0;
        qu(a, b, c, d, e);
        return null
    };
    q.mk = function(a, b) {
        switch (b) {
            case 0:
                lu(a, "Missing value for string option.")
        }
    };

    function nu(a, b) {
        switch (b) {
            case 0:
                return E(a.fl, a);
            case 2:
                return E(a.el, a);
            case 3:
                return E(a.dl, a);
            case 4:
            case 1:
                return E(a.gl, a);
            default:
                return aa()
        }
    }

    function ou(a, b) {
        switch (b) {
            case 0:
                return E(a.lk, a);
            case 2:
                return E(a.kk, a);
            case 3:
                return E(a.jk, a);
            case 4:
            case 1:
                return E(a.mk, a);
            default:
                return aa()
        }
    }

    function pu(a, b) {
        this.A = a;
        this.B = b
    };

    function ru(a) {
        this.D = null;
        this.C = [];
        this.B = null;
        this.B = a ? wa(a) ? mu(su(this), a) : a : mu(su(this), "")
    }

    function su(a) {
        null == a.D && (a.D = new iu);
        return a.D
    }

    function tu(a, b, c, d) {
        b || "number" == typeof b && 0 == b || (b = void 0);
        var e = a.B.A;
        a = a.B.B;
        var f = c.call(e);
        b != f && (void 0 != f && c.call(a), d.call(e, b))
    }
    q = ru.prototype;
    q.te = function(a) {
        tu(this, a, Y.prototype.dg, Y.prototype.Yg);
        return this
    };
    q.se = function(a) {
        tu(this, a, Y.prototype.bg, Y.prototype.Xg);
        return this
    };
    q.ue = function(a) {
        tu(this, a, Y.prototype.eg, Y.prototype.ah);
        return this
    };
    q.Id = function(a) {
        tu(this, a, Y.prototype.We, Y.prototype.Zg);
        return this
    };
    q.re = function(a) {
        tu(this, a, Y.prototype.ag, Y.prototype.Wg);
        return this
    };
    q.we = function(a) {
        tu(this, a, Y.prototype.kg, Y.prototype.eh);
        return this
    };
    q.ve = function(a) {
        tu(this, a, Y.prototype.jg, Y.prototype.bh);
        return this
    };
    q.ye = function(a) {
        tu(this, a, Y.prototype.mg, Y.prototype.hh);
        return this
    };
    q.xe = function(a) {
        tu(this, a, Y.prototype.lg, Y.prototype.gh);
        return this
    };
    q.mc = function(a) {
        tu(this, a, Y.prototype.ee, Y.prototype.dh);
        return this
    };
    q.Jd = function(a) {
        tu(this, a, Y.prototype.W, Y.prototype.lh);
        return this
    };
    q.Ic = function() {
        this.C.length = 0;
        uu(this, "s", Y.prototype.ee);
        uu(this, "w", Y.prototype.W);
        vu(this, "c", Y.prototype.dg);
        uu(this, "c", Y.prototype.ll, 16, 6);
        vu(this, "d", Y.prototype.Ti);
        uu(this, "h", Y.prototype.We);
        vu(this, "s", Y.prototype.Jj);
        vu(this, "h", Y.prototype.nl);
        vu(this, "p", Y.prototype.kg);
        vu(this, "pa", Y.prototype.tj);
        vu(this, "pd", Y.prototype.jg);
        vu(this, "pp", Y.prototype.lg);
        vu(this, "pf", Y.prototype.mg);
        uu(this, "p", Y.prototype.Wi);
        vu(this, "n", Y.prototype.ag);
        uu(this, "r", Y.prototype.Gj);
        vu(this, "r", Y.prototype.Ij);
        vu(this, "fh", Y.prototype.bj);
        vu(this, "fv", Y.prototype.Qj);
        vu(this, "cc", Y.prototype.bg);
        vu(this, "ci", Y.prototype.eg);
        vu(this, "o", Y.prototype.ql);
        wu(this, "o", Y.prototype.K);
        wu(this, "j", Y.prototype.I);
        uu(this, "x", Y.prototype.ng);
        uu(this, "y", Y.prototype.og);
        uu(this, "z", Y.prototype.Vg);
        vu(this, "g", Y.prototype.Nj);
        vu(this, "fg", Y.prototype.Zi);
        vu(this, "ft", Y.prototype.$i);
        uu(this, "e", Y.prototype.ml);
        wu(this, "f", Y.prototype.dj);
        vu(this, "k", Y.prototype.gj);
        uu(this, "k", Y.prototype.Xi);
        vu(this, "u", Y.prototype.qg);
        vu(this, "ut", Y.prototype.tg);
        vu(this, "i", Y.prototype.pl);
        vu(this, "ip", Y.prototype.ej);
        vu(this, "a", Y.prototype.Ui);
        uu(this, "a", Y.prototype.Hj);
        uu(this, "m", Y.prototype.Sj);
        uu(this, "vb", Y.prototype.Rj);
        uu(this, "vl", Y.prototype.Tj);
        vu(this, "lf", Y.prototype.hj);
        vu(this, "mv", Y.prototype.ij);
        vu(this, "id", Y.prototype.cj);
        uu(this, "ic", Y.prototype.fj);
        vu(this, "b", Y.prototype.Ri);
        uu(this, "b", Y.prototype.Qi);
        wu(this, "t", Y.prototype.rl);
        wu(this, "nt0", Y.prototype.Pj);
        vu(this, "rw", Y.prototype.Dj);
        vu(this, "rwu",
            Y.prototype.Ej);
        vu(this, "rwa", Y.prototype.yj);
        vu(this, "nw", Y.prototype.pj);
        vu(this, "rh", Y.prototype.Aj);
        vu(this, "nc", Y.prototype.kj);
        vu(this, "nd", Y.prototype.lj);
        vu(this, "no", Y.prototype.mj);
        wu(this, "q", Y.prototype.xj);
        vu(this, "ns", Y.prototype.nj);
        uu(this, "l", Y.prototype.wj);
        uu(this, "v", Y.prototype.vj);
        vu(this, "nu", Y.prototype.oj);
        vu(this, "rj", Y.prototype.Bj);
        vu(this, "rp", Y.prototype.Cj);
        vu(this, "rg", Y.prototype.zj);
        vu(this, "pg", Y.prototype.Oj);
        vu(this, "mo", Y.prototype.jj);
        vu(this, "al", Y.prototype.Ni);
        uu(this, "iv", Y.prototype.ol);
        uu(this, "pi", Y.prototype.sj);
        uu(this, "ya", Y.prototype.Vj);
        uu(this, "ro", Y.prototype.Fj);
        uu(this, "fo", Y.prototype.aj);
        vu(this, "df", Y.prototype.Si);
        wu(this, "mm", Y.prototype.Uj);
        vu(this, "sg", Y.prototype.Kj);
        vu(this, "gd", Y.prototype.uj);
        vu(this, "fm", Y.prototype.Yi);
        uu(this, "ba", Y.prototype.Oi);
        uu(this, "br", Y.prototype.Pi);
        uu(this, "bc", Y.prototype.kl, 16, 6);
        uu(this, "pc", Y.prototype.qj, 16, 6);
        uu(this, "sc", Y.prototype.Mj, 16, 6);
        return this.C.join("-")
    };

    function xu(a, b) {
        if (void 0 == b) return "";
        a = b - a.length;
        return 0 >= a ? "" : ab("0", a)
    }

    function uu(a, b, c, d, e) {
        var f = c.call(a.B.A);
        if (void 0 != f && null != f) {
            d = void 0 == d ? 10 : 10 != d && 16 != d ? 10 : d;
            var f = f.toString(d),
                g = new vt;
            g.Vd(16 == d ? "0x" : "");
            g.Vd(xu(f, e));
            g.Vd(f);
            yu(a, b, g.toString(), c)
        }
    }

    function vu(a, b, c) {
        c.call(a.B.A) && yu(a, b, "", c)
    }

    function wu(a, b, c) {
        var d = c.call(a.B.A);
        d && yu(a, b, d, c)
    }

    function yu(a, b, c, d) {
        d.call(a.B.B) && (b = b.substring(0, 1).toUpperCase() + b.substring(1));
        a.C.push(b + c)
    };

    function zu(a) {
        ru.call(this, a)
    }
    H(zu, ru);
    q = zu.prototype;
    q.te = function(a) {
        a && Au(this);
        return zu.V.te.call(this, a)
    };
    q.Id = function(a) {
        null != a && this.mc();
        return zu.V.Id.call(this, a)
    };
    q.ue = function(a) {
        a && Au(this);
        return zu.V.ue.call(this, a)
    };
    q.se = function(a) {
        a && Au(this);
        return zu.V.se.call(this, a)
    };
    q.mc = function(a) {
        za(a) && (a = Math.max(a.width, a.height));
        null != a && (this.Jd(), this.Id());
        return zu.V.mc.call(this, a)
    };
    q.we = function(a) {
        a && Au(this);
        return zu.V.we.call(this, a)
    };
    q.xe = function(a) {
        a && Au(this);
        return zu.V.xe.call(this, a)
    };
    q.ye = function(a) {
        a && Au(this);
        return zu.V.ye.call(this, a)
    };
    q.re = function(a) {
        a && Au(this);
        return zu.V.re.call(this, a)
    };
    q.ve = function(a) {
        a && Au(this);
        return zu.V.ve.call(this, a)
    };
    q.Jd = function(a) {
        null != a && this.mc();
        return zu.V.Jd.call(this, a)
    };

    function Au(a) {
        a.re();
        a.se();
        a.te();
        a.ue();
        a.ve();
        a.we();
        a.xe();
        a.ye()
    }
    q.Ic = function() {
        var a = this.B.A;
        a.qg() || a.tg() ? a.ee() || this.mc(0) : (a = this.B.A, a.ee() || a.W() || a.We() || (this.mc(), this.Id(), this.Jd(), Au(this)));
        return zu.V.Ic.call(this)
    };
    var Bu = /^[^\/]*\/\//;

    function Cu() {}

    function Du(a) {
        this.C = a;
        this.de = "";
        (a = this.C.match(Bu)) && a[0] ? (this.de = a[0], a = this.de.match(/\w+/) ? this.C : "http://" + this.C.substring(this.de.length)) : a = "http://" + this.C;
        this.Jc = a instanceof lr ? new lr(a) : new lr(a, !0);
        this.D = !0;
        this.M = !1
    }

    function Eu(a, b) {
        a.B = a.B ? a.B + ("/" + b) : b
    }

    function Fu(a) {
        if (void 0 == a.A) {
            a.B = null;
            a.A = a.Jc.D.substring(1).split("/");
            var b = a.A.length;
            2 < b && Ja(a.Jc.C, "google.com") && "u" == a.A[0] && (Eu(a, a.A[0] + "/" + a.A[1]), a.A.shift(), a.A.shift(), b -= 2);
            if (0 == b || 4 == b || 7 < b) return a.D = !1, a.A;
            if (2 == b) Eu(a, a.A[0]);
            else if ("image" == a.A[0]) Eu(a, a.A[0]);
            else if (7 == b || 3 == b) return a.D = !1, a.A;
            if (3 >= b) {
                a.M = !0;
                3 == b && (Eu(a, a.A[1]), a.A.shift(), --b);
                var b = b - 1,
                    c = a.A[b],
                    d = c.indexOf("="); - 1 != d && (a.A[b] = c.substr(0, d), a.A.push(c.substr(d + 1)))
            }
        }
        return a.A
    }

    function Gu(a) {
        Fu(a);
        return a.M
    }

    function Hu(a) {
        Fu(a);
        void 0 == a.B && (a.B = null);
        return a.B
    }

    function Iu(a) {
        switch (Fu(a).length) {
            case 7:
                return !0;
            case 6:
                return null == Hu(a);
            case 5:
                return !1;
            case 3:
                return !0;
            case 2:
                return null == Hu(a);
            case 1:
                return !1;
            default:
                return !1
        }
    }

    function Ju(a, b) {
        if (Gu(a)) a: {
            var c = null != Hu(a) ? 1 : 0;
            switch (b) {
                case 6:
                    b = 0 + c;
                    break;
                case 4:
                    if (!Iu(a)) {
                        a = null;
                        break a
                    }
                    b = 1 + c;
                    break;
                default:
                    a = null;
                    break a
            }
            a = Fu(a)[b]
        }
        else a: {
            c = null != Hu(a) ? 1 : 0;
            switch (b) {
                case 0:
                    b = 0 + c;
                    break;
                case 1:
                    b = 1 + c;
                    break;
                case 2:
                    b = 2 + c;
                    break;
                case 3:
                    b = 3 + c;
                    break;
                case 4:
                    if (!Iu(a)) {
                        a = null;
                        break a
                    }
                    b = 4 + c;
                    break;
                case 5:
                    b = Iu(a) ? 1 : 0;
                    b = 4 + c + b;
                    break;
                default:
                    a = null;
                    break a
            }
            a = Fu(a)[b]
        }
        return a
    }

    function Ku(a) {
        void 0 == a.J && (a.J = Ju(a, 0));
        return a.J
    }

    function Lu(a) {
        void 0 == a.L && (a.L = Ju(a, 1));
        return a.L
    }

    function Mu(a) {
        void 0 == a.I && (a.I = Ju(a, 2));
        return a.I
    }

    function Nu(a) {
        void 0 == a.N && (a.N = Ju(a, 3));
        return a.N
    }

    function Ou(a) {
        void 0 == a.F && (a.F = Ju(a, 5));
        return a.F
    };

    function Pu(a) {
        this.A = null;
        a instanceof Du ? this.A = a : (void 0 == Qu && (Qu = new Cu), this.A = new Du(a.toString()));
        a = this.A;
        if (void 0 == a.H) {
            var b;
            void 0 == a.G && (a.G = Ju(a, 4));
            (b = a.G) || (b = "");
            a.H = mu(new iu, b)
        }
        ru.call(this, a.H);
        this.H = this.A.de;
        a = this.A;
        b = a.Jc.H;
        this.G = a.Jc.C + (b ? ":" + b : "");
        this.F = this.A.Jc.A.toString()
    }
    var Qu;
    H(Pu, zu);
    Pu.prototype.Ic = function() {
        var a = this.A;
        Fu(a);
        if (!a.D) return this.A.C;
        var a = Pu.V.Ic.call(this),
            b = [];
        null != Hu(this.A) && b.push(Hu(this.A));
        if (Gu(this.A)) {
            var c = this.A;
            void 0 == c.K && (c.K = Ju(c, 6));
            b.push(c.K + (a ? "=" + a : ""))
        } else b.push(Ku(this.A)), b.push(Lu(this.A)), b.push(Mu(this.A)), b.push(Nu(this.A)), a && b.push(a), b.push(Ou(this.A));
        return this.H + this.G + "/" + b.join("/") + (this.F ? "?" + this.F : "")
    };
    var Ru = /^((http(s)?):)?\/\/((((lh[3-6](-tt|-d[a-g,z])?\.((ggpht)|(googleusercontent)|(google)))|(([1-4]\.bp\.blogspot)|(bp[0-3]\.blogger))|((((cp|ci|gp)[3-6])|(ap[1-2]))\.(ggpht|googleusercontent))|(gm[1-4]\.ggpht)|(((yt[3-4])|(sp[1-3]))\.(ggpht|googleusercontent)))\.com)|(dp[3-6]\.googleusercontent\.cn)|(lh[3-6]\.(googleadsserving\.cn|xn--9kr7l\.com))|(photos\-image\-(dev|qa)(-auth)?\.corp\.google\.com)|((dev|dev2|dev3|qa|qa2|qa3|qa-red|qa-blue|canary)[-.]lighthouse\.sandbox\.google\.com\/image)|(image\-dev\-lighthouse(-auth)?\.sandbox\.google\.com(\/image)?))\//i,
        Su = /^(https?:)?\/\/sp[1-4]\.((ggpht)|(googleusercontent))\.com\//i,
        Tu = /^(https?:)?\/\/(qa(-red|-blue)?|dev2?|image-dev)(-|\.)lighthouse(-auth)?\.sandbox\.google\.com\//i,
        Uu = /^(https?:)?\/\/lighthouse-(qa(-red|-blue)?|dev2)\.corp\.google\.com\//i;

    function Vu(a, b, c) {
        this.D = a;
        this.C = b;
        this.G = c;
        this.A = new nl(5)
    }
    Vu.prototype.B = function(a) {
        var b = Ns(a),
            c, d;
        Ru.test(b) || Su.test(b) || Tu.test(b) || Uu.test(b) ? (c = b.substr(0, b.lastIndexOf("/")), d = b.substr(b.lastIndexOf("/") + 1)) : (c = this.G[0] + b, d = "p", b = c + "/" + d);
        var e = pl(this.A, b);
        e || (e = d, d = new st(this.D, this.C, c, e), c = this.F || new rt(this.D, c, e), e = new tt(c, d, this.C), ol(this.A, b, e));
        a = new Wu(e, a);
        a.C = Ep;
        a.Jb();
        return a
    };
    Vu.prototype.clear = function() {
        this.A.clear()
    };

    function Xu() {
        this.H = !1;
        this.F = null;
        this.L = new le;
        this.M = El();
        this.O = El();
        this.A = this.B = this.C = this.G = this.N = 0;
        this.D = new oe;
        this.K = new oe;
        this.I = new oe;
        this.J = new oe
    }
    var pq = rl(),
        qq = rl(),
        rq = rl(),
        sq = rl(),
        Op = El(),
        Pp = El(),
        Qp = new Dj;

    function Bn(a, b) {
        return Math.ceil(a / b) * b
    }

    function Yu() {
        return 0
    }

    function Np(a, b, c) {
        if (c) {
            var d = new le;
            U(d, a.L);
            Cs(c, d);
            Zu(d, b)
        } else Fl(b, a.M)
    }

    function tn(a, b) {
        var c = a.J.W(),
            d = a.K.W(),
            e = a.I.W(),
            f = 1,
            g = 0;
        c && d && (f = d / c, e && (g = e / c));
        return g + Math.min(b / a.C, 1) * f
    }

    function un(a, b) {
        var c = Ce(a.J),
            d = Ce(a.K),
            e = Ce(a.I),
            f = 1,
            g = 0;
        c && d && (f = d / c, e && (g = e / c));
        return g + Math.min(b / a.B, 1) * f
    }

    function zn(a, b, c, d, e) {
        var f;
        f = a.F;
        var g = b * a.F.W();
        a = yp(f.D, g, c * a.F.B);
        f = Hp(f, a);
        0 == f && (f = 500);
        b = Math.PI * (2 * b - 1);
        c = Math.PI * (.5 - c);
        d[e + 0] = Math.sin(b) * Math.cos(c) * f;
        d[e + 1] = Math.cos(b) * Math.cos(c) * f;
        d[e + 2] = Math.sin(c) * f
    }

    function up(a, b, c, d) {
        Pl(c, b.A, pq);
        yl(pq, pq);
        d.x = Math.atan2(pq[0], pq[1]) / (2 * Math.PI) + .5;
        d.y = Math.acos(pq[2]) / Math.PI
    }

    function Zu(a, b) {
        var c = new zm;
        km(a, c);
        var d = c.A,
            e = c.B,
            c = c.C;
        b[0] = 1;
        b[1] = 0;
        b[2] = 0;
        b[3] = 0;
        b[4] = 0;
        b[5] = 1;
        b[6] = 0;
        b[7] = 0;
        b[8] = 0;
        b[9] = 0;
        b[10] = 1;
        b[11] = 0;
        b[12] = d;
        b[13] = e;
        b[14] = c;
        b[15] = 1;
        d = cm(xe(qe(a)));
        Ul(b, d, d, d);
        a = se(a);
        Xl(b, Jb(-Ae(a)));
        Vl(b, Jb(N(a, 1) - 90));
        Wl(b, Jb(N(a, 2)))
    };

    function $u() {
        this.C = this.A = 0;
        this.B = null;
        this.D = 0;
        this.G = [];
        this.F = {};
        this.H = {}
    }
    $u.prototype.W = h("A");

    function av(a, b, c) {
        il.call(this);
        this.J = "" + Aa(this);
        this.L = a;
        this.C = c;
        this.H = [c];
        this.I = !1;
        this.A = new Float32Array(12);
        this.B = 0;
        this.G = null;
        this.F = this.D = 1;
        this.Aa();
        c = 1 * (this.I ? 2 : 1);
        var d = 2 * c / 1.25 / 80,
            e = 1.25 * c;
        a = this.A;
        a[0] = c;
        a[1] = 7.5;
        a[2] = -3;
        a[3] = -c;
        a[4] = 7.5;
        a[5] = -3;
        a[6] = e;
        a[7] = this.B * d + 7.5;
        a[8] = -3;
        a[9] = -e;
        a[10] = this.B * d + 7.5;
        a[11] = -3;
        sk(bv, -Jb(b));
        for (b = 0; 4 > b; b++) {
            cv[0] = a[3 * b + 0];
            cv[1] = a[3 * b + 1];
            cv[2] = a[3 * b + 2];
            c = bv;
            var f = cv,
                d = cv,
                e = f[0],
                g = f[1],
                f = f[2];
            d[0] = e * c[0] + g * c[4] + f * c[8];
            d[1] = e * c[1] + g * c[5] + f * c[9];
            d[2] = e * c[2] + g * c[6] + f * c[10];
            a[3 * b + 0] = cv[0];
            a[3 * b + 1] = cv[1];
            a[3 * b + 2] = cv[2]
        }
    }
    H(av, il);
    var cv = new Float32Array(3),
        bv = ok();
    q = av.prototype;
    q.sb = ca(2);

    function dv(a) {
        for (var b = 1; b < a;) b <<= 1;
        return b
    }
    q.Da = ca(null);
    q.fa = ca(null);
    q.id = h("J");
    q.Rb = C;
    q.dc = function(a) {
        a(3)
    };
    q.ma = h("L");
    q.Aa = function() {
        if (!this.G) {
            var a;
            if (!this.C || 1 > this.C.length) a = null;
            else {
                a = Nj("CANVAS");
                var b, c;
                a.getContext ? (c = a.getContext("2d"), b = E(c.measureText, c)) : (c = null, b = ev);
                fv(c);
                var d = this.C,
                    e = [],
                    f = "",
                    g = d,
                    k;
                c ? k = E(c.measureText, c) : k = ev;
                if (1024 < k(d).width)
                    for (var d = d.split(" "), l = 0, m = 1, n = 0; n < d.length && l < m; n++) f = f + d[n] + " ", g = g.substring(d[n].length + 1), l = k(f).width, m = k(g).width;
                f && e.push(f);
                g && e.push(g);
                this.H = e;
                f = 0;
                g = 100 * e.length;
                0 != (g & g - 1) && (k = dv(g), this.D = g / k, g = k);
                a.height = g;
                fv(c);
                b = b(e[0]);
                this.B = f = Math.max(f,
                    b.width);
                0 != (f & f - 1) && (b = dv(f), this.F = f / b, f = b);
                a.width = f;
                fv(c);
                c && (c.strokeText(e[0], 0, 0), c.fillText(e[0], 0, 0), e[1] && (this.I = !0, c.strokeText(e[1], 0, 100), c.fillText(e[1], 0, 100)))
            }
            this.G = a
        }
        return this.G
    };

    function ev(a, b, c) {
        b = b || "Arial";
        c = c || 80;
        var d = Nj("dummyContainer");
        w.document.body.appendChild(d);
        var e = Rj("dummyText");
        Lj(e, {
            style: "font-family:" + b + ";position:absolute;top:-20000px;left:-20000px;padding:0;margin:0;border:0;white-space:pre;font-size:" + c + "px"
        });
        e.appendChild(document.createTextNode(String(a)));
        d.appendChild(e);
        c = Jj(e);
        a = new Dj(0, 0);
        b = c ? Jj(c) : document;
        b = !Rc || 9 <= Number(fd) || "CSS1Compat" == Hj(b).A.compatMode ? b.documentElement : b.body;
        if (e != b) {
            b = Vn(e);
            var f = Hj(c).A;
            c = f.scrollingElement ?
                f.scrollingElement : Uc || "CSS1Compat" != f.compatMode ? f.body || f.documentElement : f.documentElement;
            f = f.parentWindow || f.defaultView;
            c = Rc && ed("10") && f.pageYOffset != c.scrollTop ? new Dj(c.scrollLeft, c.scrollTop) : new Dj(f.pageXOffset || c.scrollLeft, f.pageYOffset || c.scrollTop);
            a.x = b.left + c.x;
            a.y = b.top + c.y
        }
        b = Yn(e);
        a = new Qn(a.x, a.y, b.width, b.height);
        d.removeChild(e);
        w.document.body.removeChild(d);
        return {
            width: a.width
        }
    }

    function fv(a) {
        a && (a.fillStyle = "rgba(255, 255, 255, 0.7)", a.font = "bold 80px Arial", a.textBaseline = "top", a.strokeStyle = "rgba(0, 0, 0, 0.15)", a.lineWidth = 2, a.shadowOffsetX = -1.5, a.shadowOffsetY = -1.5, a.shadowBlur = 4, a.shadowColor = "rgba(0, 0, 0, 0.5)")
    };

    function gv(a) {
        il.call(this);
        this.D = 0;
        this.X = a;
        this.F = {};
        this.A = {};
        this.Y = "" + Aa(this);
        this.L = this.J = !1;
        this.N = new Vs;
        this.P = null;
        this.G = []
    }
    H(gv, il);
    q = gv.prototype;
    q.sb = h("D");

    function hv(a, b) {
        if (b != a.D && (0 == b || 4 !== a.D) && (a.D = b, Zo(a) || $o(a))) {
            b = a.G;
            a.G = [];
            for (var c = 0; c < b.length; ++c)(0, b[c])(a.D)
        }
    }
    q.dc = function(a) {
        Zo(this) || $o(this) ? a(this.D) : this.G.push(a)
    };
    q.Rb = function(a) {
        this.Gb(0, 0, Yu(this.ma()), a, "pfdd");
        this.Ac(a, "pfdd")
    };
    q.Da = h("N");
    q.Ib = function(a) {
        U(Zs(this.N), a)
    };
    q.id = h("Y");
    q.Uc = function(a) {
        return !!this.F[a]
    };
    q.tb = function(a, b, c) {
        return this.F[a + "|" + b + "|" + c] || null
    };
    q.Hd = function(a, b) {
        var c = a.toString();
        this.F[c] = a;
        this.A[c] && delete this.A[c];
        1 === this.sb() && hv(this, 2);
        To(this, "TileReady", b, new Zp("TileReady", this, a.C, a.D, a.B))
    };
    q.Gd = function(a, b) {
        var c = a.toString();
        this.F[c] = a;
        this.A[c] && delete this.A[c];
        c = this.sb();
        1 === c ? (hv(this, 2), hv(this, 3)) : 2 === c && hv(this, 3);
        To(this, "TileReady", b, new Zp("TileReady", this, a.C, a.D, a.B))
    };
    q.Ac = function(a, b) {
        this.J || 0 !== this.sb() || (this.J = !0, this.X.Ac(this, a, b))
    };
    q.Kc = function() {
        this.L = !0;
        for (var a in this.A) this.A[a]();
        hv(this, 0);
        for (this.J = !1; this.G.length;) this.G.shift()(0);
        this.L = !1;
        this.A = {}
    };
    q.he = function() {
        hv(this, 4)
    };
    q.Gb = function(a, b, c, d, e) {
        var f = a + "|" + b + "|" + c;
        this.F[f] || this.A[f] || (this.A[f] = this.X.Gb(this, a, b, c, d, e))
    };
    q.Fc = function(a, b, c) {
        a = a + "|" + b + "|" + c;
        this.A[a] && delete this.A[a]
    };
    q.Dd = function(a, b, c) {
        a = a + "|" + b + "|" + c;
        this.A[a] && delete this.A[a]
    };
    q.Je = function() {
        for (var a in this.A) return !0;
        return !1
    };
    q.Le = h("L");
    q.ea = function() {
        gv.V.ea.call(this);
        this.F = {};
        this.Je() && this.Kc()
    };
    q.je = function(a) {
        this.P = a;
        var b = Ah(Zs(this.N)),
            c = qh(b);
        U(b, a);
        K(c, 8) && !K(qh(b), 8) && U(new hf(P(rh(b), 8)), c.eb())
    };
    q.ia = h("P");
    q.Nb = function() {
        return []
    };

    function Wu(a, b) {
        gv.call(this, a);
        this.Ib(b);
        this.C = this.B = null;
        this.O = new le;
        this.$ = new $u;
        this.H = new Xu;
        this.I = null;
        this.U = !1
    }
    H(Wu, gv);
    var iv = new ym,
        jv = El(),
        kv = rl(),
        lv = rl();
    q = Wu.prototype;
    q.fa = h("O");
    q.Ha = h("B");
    q.Qd = function(a, b) {
        var c = Eb(a.y, 0, 1),
            d = Math.round(Eb(a.x, 0, 1) * (this.Cb().W() - 1)),
            e = this.Cb(),
            c = Math.round(c * (this.Cb().C - 1)),
            d = !e.B || 0 > d || 0 > c || d >= e.A || c >= e.C ? "" : e.G[Kp(e.B, e.D + (c * e.A + d))] || "";
        if (!d) return null;
        Jc(b, 8);
        b.data[0] = d;
        e = Bh(b);
        (c = this.Cb().H[d]) ? (U(re(e), c), e = !0) : e = !1;
        if (!e) return null;
        (a = Gp(this.Pd(), a.x, a.y, iv)) ? (Np(this.ma(), jv), Ol(jv, a.origin, kv), a = kv) : a = null;
        a && dp(Bh(b), a);
        a = Bh(b);
        e = new zm;
        km(a, e);
        sl(kv, e.A, e.B, e.C);
        Np(this.ma(), jv);
        c = El();
        Nl(jv, c);
        Ol(c, kv, kv);
        if (c = this.C) {
            c = this.C;
            kv[2] = 999999;
            sl(Bp, 0, 0, -1);
            for (var f = new ym(kv, Bp), g = 0, k = 4; k < c.A.length; k += 4) {
                var l = Dp,
                    m = c.A[k + 1],
                    n = c.A[k + 2],
                    p = c.A[k + 3];
                l[0] = c.A[k];
                l[1] = m;
                l[2] = n;
                l[3] = p;
                .6 > zl(Dp, f.A) || (l = f, m = Cp, n = Dp, p = zl(n, l.A), n = 1E-6 > Math.abs(p) ? Infinity : -(zl(n, l.origin) + n[3]) / p, !isFinite(n) || 0 > n ? l = null : (wl(l.A, n, m), ul(l.origin, m, m), l = n), null !== l && (Cp[2] -= 7, m = c, m.A ? (yl(Cp, Bp), n = xp(m.D, Bp), m = Jp(m, n[0], n[1])) : m = -1, m == k / 4 && l > g && (tl(lv, Cp), g = l)))
            }
            c = 0 < g
        }
        c ? (c = new ql, Ol(jv, lv, lv), c.C = lv[2], lm(e, c), mm(e, a)) : ze(re(a), ye(qe(this.fa())) - 3);
        switch (this.Cb().F[d]) {
            case 3:
                b.data[1] = 4;
                break;
            default:
                b.data[1] = 0
        }
        d = 1;
        K(Ys(this.Da()), 2) && (d = zh(Ys(this.Da())));
        b.data[2] = d;
        return b
    };
    q.Cb = h("$");
    q.Pd = h("C");
    q.ma = function() {
        this.H.H || this.Jb();
        return this.H
    };
    q.Jb = function() {
        if (this.B && this.C) {
            var a = this.H,
                b = this.B;
            a.F = this.C;
            a.L = b.fa();
            Zu(a.L, a.M);
            Nl(a.M, a.O);
            U(a.D, new oe(b.data[3]));
            var c = ue(b.fa());
            a.C = c.W() / a.D.W();
            a.B = Ce(c) / Ce(a.D);
            a.N = Math.ceil(a.C);
            a.G = Math.ceil(a.B);
            a.A = N(b, 5);
            U(a.J, new oe(b.data[26]));
            U(a.K, new oe(b.data[27]));
            U(a.I, new oe(b.data[28]));
            a.H = !0
        }
        mv(this)
    };
    q.Nb = function() {
        if (!this.B) return [];
        if (!this.I) {
            this.I = [];
            for (var a = S(this.B, 19), b = 0; b < a; b++) {
                var c = new Qs(Nc(this.B, 19, b)),
                    d = N(c, 0) - N(this.B, 10);
                a: {
                    var e = O(c, 3),
                        c = O(this.B, 13),
                        f = e.split("/");
                    if (1 == f.length) c = e;
                    else {
                        for (e = 0; e < f.length; ++e) {
                            var g = La(f[e]);
                            if (g == c) {
                                c = g;
                                break a
                            }
                        }
                        c = La(f[0])
                    }
                }
                c && (d = new av(this.H, d, c), this.I.push(d))
            }
        }
        return this.I
    };
    q.Tb = function(a, b) {
        this.B = a;
        var c = new wh;
        U(c, Ys(this.Da()));
        var d = !1;
        O(a, 0) && c.za() != O(a, 0) && (c.data[0] = O(a, 0), Re(new Ne(P(Ah(c), 1)), O(a, 0)), d = !0);
        var e = 1;
        K(a, 6) && (e = L(a, 6, 1));
        K(a, 13) && 2 == e && (c.data[3] = O(a, 13));
        c.data[2] = e;
        this.Ib(c);
        d ? (hv(this, 0), mv(this), this.Dd(0, 0, 0), this.Gb(0, 0, 0, b)) : this.U && hv(this, 4);
        var f;
        0 < S(c.ia(), 5) && (f = uh(c.ia(), 0));
        f && K(Tg(f), 2) && (b = new Xe(Tg(f).data[2]), K(b, 0) && (te(Rs(a)).data[0] = N(b, 0)), K(b, 1) && (te(Rs(a)).data[1] = N(b, 1, 90)), K(b, 2) && (te(Rs(a)).data[2] = N(b, 2)));
        U(this.O,
            a.fa());
        c = qh(c.ia());
        b = new gf(c.data[4]);
        K(b, 0) && (d = new ld(b.data[0]), Be(new oe(P(a, 26)), d.W()), De(new oe(P(a, 26)), N(d, 0)));
        K(b, 1) && (d = new ld(b.data[1]), Be(new oe(P(a, 28)), d.W()), De(new oe(P(a, 28)), N(d, 0)));
        K(c, 2) && (d = new ld(c.data[2]), Be(new oe(P(a, 27)), d.W()), De(new oe(P(a, 27)), N(d, 0)));
        Ic(a, 1) ? hv(this, 4) : this.Jb()
    };
    q.Fc = function(a, b, c) {
        Wu.V.Fc.call(this, a, b, c);
        0 == a && 0 == b && 0 == c && (this.B ? hv(this, 4) : this.U = !0)
    };

    function mv(a) {
        Zo(a) || $o(a) || a.B && a.C && a.H.H && (a.Uc("0|0|0") ? hv(a, 2) : hv(a, 1))
    };

    function nv(a, b, c, d) {
        Vu.call(this, a, b, c);
        this.F = d
    }
    H(nv, Vu);

    function ov(a, b, c, d, e, f) {
        b = new nv(c, d, e, f);
        a(b)
    };

    function pv() {
        this.F = null;
        this.C = !1;
        this.A = {};
        this.B = {}
    }
    pv.prototype.tb = function(a, b, c, d, e, f, g) {
        var k = "x" + b + "-y" + c + "-z" + d;
        return this.C ? this.D(0, b, c, d, e, f, g) : (this.A[k] = E(this.D, this, a, b, c, d, e, f, g), E(this.I, this, k))
    };
    pv.prototype.I = function(a) {
        a in this.A ? delete this.A[a] : a in this.B && (this.B[a](), delete this.B[a])
    };
    pv.prototype.Tb = function(a) {
        this.C = !0;
        this.F = a;
        for (var b in this.A) a = this.A[b](), this.B[b] = a;
        this.A = {}
    };
    pv.prototype.Ha = h("F");

    function qv(a, b) {
        il.call(this);
        this.B = a || 1;
        this.A = b || w;
        this.C = E(this.Sm, this);
        this.D = F()
    }
    H(qv, il);
    q = qv.prototype;
    q.Td = !1;
    q.ub = null;
    q.Sm = function() {
        if (this.Td) {
            var a = F() - this.D;
            0 < a && a < .8 * this.B ? this.ub = this.A.setTimeout(this.C, this.B - a) : (this.ub && (this.A.clearTimeout(this.ub), this.ub = null), this.dispatchEvent("tick"), this.Td && (this.ub = this.A.setTimeout(this.C, this.B), this.D = F()))
        }
    };
    q.start = function() {
        this.Td = !0;
        this.ub || (this.ub = this.A.setTimeout(this.C, this.B), this.D = F())
    };
    q.ea = function() {
        qv.V.ea.call(this);
        this.Td = !1;
        this.ub && (this.A.clearTimeout(this.ub), this.ub = null);
        delete this.A
    };

    function rv(a, b, c) {
        if (ya(a)) c && (a = E(a, c));
        else if (a && "function" == typeof a.handleEvent) a = E(a.handleEvent, a);
        else throw Error("Invalid listener argument");
        return 2147483647 < Number(b) ? -1 : w.setTimeout(a, b || 0)
    };

    function sv() {}
    sv.prototype.A = null;

    function tv(a) {
        var b;
        (b = a.A) || (b = {}, uv(a) && (b[0] = !0, b[1] = !0), b = a.A = b);
        return b
    };
    var vv;

    function wv() {}
    H(wv, sv);

    function xv(a) {
        return (a = uv(a)) ? new ActiveXObject(a) : new XMLHttpRequest
    }

    function uv(a) {
        if (!a.B && "undefined" == typeof XMLHttpRequest && "undefined" != typeof ActiveXObject) {
            for (var b = ["MSXML2.XMLHTTP.6.0", "MSXML2.XMLHTTP.3.0", "MSXML2.XMLHTTP", "Microsoft.XMLHTTP"], c = 0; c < b.length; c++) {
                var d = b[c];
                try {
                    return new ActiveXObject(d), a.B = d
                } catch (e) {}
            }
            throw Error("Could not create ActiveXObject. ActiveX might be disabled, or MSXML might not be installed");
        }
        return a.B
    }
    vv = new wv;

    function yv(a) {
        il.call(this);
        this.headers = new $i;
        this.L = a || null;
        this.B = !1;
        this.J = this.A = null;
        this.H = this.Y = this.U = "";
        this.C = this.P = this.G = this.N = !1;
        this.I = 0;
        this.F = null;
        this.D = "";
        this.X = this.O = !1
    }
    H(yv, il);
    var zv = /^https?$/i,
        Av = ["POST", "PUT"];

    function Bv(a, b, c, d, e) {
        if (a.A) throw Error("[goog.net.XhrIo] Object is active with another request=" + a.U + "; newUri=" + b);
        c = c ? c.toUpperCase() : "GET";
        a.U = b;
        a.H = "";
        a.Y = c;
        a.N = !1;
        a.B = !0;
        a.A = a.L ? xv(a.L) : xv(vv);
        a.J = a.L ? tv(a.L) : tv(vv);
        a.A.onreadystatechange = E(a.Ng, a);
        try {
            a.P = !0, a.A.open(c, String(b), !0), a.P = !1
        } catch (g) {
            Cv(a, g);
            return
        }
        b = d || "";
        var f = new $i(a.headers);
        e && ej(e, function(a, b) {
            f.set(b, a)
        });
        e = qb(f.kb(), Dv);
        d = w.FormData && b instanceof w.FormData;
        !sb(Av, c) || e || d || f.set("Content-Type", "application/x-www-form-urlencoded;charset=utf-8");
        f.forEach(function(a, b) {
            this.A.setRequestHeader(b, a)
        }, a);
        a.D && (a.A.responseType = a.D);
        "withCredentials" in a.A && a.A.withCredentials !== a.O && (a.A.withCredentials = a.O);
        try {
            Ev(a), 0 < a.I && (a.X = Fv(a.A), a.X ? (a.A.timeout = a.I, a.A.ontimeout = E(a.Hb, a)) : a.F = rv(a.Hb, a.I, a)), a.G = !0, a.A.send(b), a.G = !1
        } catch (g) {
            Cv(a, g)
        }
    }

    function Fv(a) {
        return Rc && ed(9) && xa(a.timeout) && y(a.ontimeout)
    }

    function Dv(a) {
        return "content-type" == a.toLowerCase()
    }
    q = yv.prototype;
    q.Hb = function() {
        "undefined" != typeof oa && this.A && (this.H = "Timed out after " + this.I + "ms, aborting", this.dispatchEvent("timeout"), this.abort(8))
    };

    function Cv(a, b) {
        a.B = !1;
        a.A && (a.C = !0, a.A.abort(), a.C = !1);
        a.H = b;
        Gv(a);
        Hv(a)
    }

    function Gv(a) {
        a.N || (a.N = !0, a.dispatchEvent("complete"), a.dispatchEvent("error"))
    }
    q.abort = function() {
        this.A && this.B && (this.B = !1, this.C = !0, this.A.abort(), this.C = !1, this.dispatchEvent("complete"), this.dispatchEvent("abort"), Hv(this))
    };
    q.ea = function() {
        this.A && (this.B && (this.B = !1, this.C = !0, this.A.abort(), this.C = !1), Hv(this, !0));
        yv.V.ea.call(this)
    };
    q.Ng = function() {
        this.wa() || (this.P || this.G || this.C ? Iv(this) : this.Xk())
    };
    q.Xk = function() {
        Iv(this)
    };

    function Iv(a) {
        if (a.B && "undefined" != typeof oa && (!a.J[1] || 4 != Jv(a) || 2 != a.Ea()))
            if (a.G && 4 == Jv(a)) rv(a.Ng, 0, a);
            else if (a.dispatchEvent("readystatechange"), 4 == Jv(a)) {
            a.B = !1;
            try {
                if (Kv(a)) a.dispatchEvent("complete"), a.dispatchEvent("success");
                else {
                    var b;
                    try {
                        b = 2 < Jv(a) ? a.A.statusText : ""
                    } catch (c) {
                        b = ""
                    }
                    a.H = b + " [" + a.Ea() + "]";
                    Gv(a)
                }
            } finally {
                Hv(a)
            }
        }
    }

    function Hv(a, b) {
        if (a.A) {
            Ev(a);
            var c = a.A,
                d = a.J[0] ? C : null;
            a.A = null;
            a.J = null;
            b || a.dispatchEvent("ready");
            try {
                c.onreadystatechange = d
            } catch (e) {}
        }
    }

    function Ev(a) {
        a.A && a.X && (a.A.ontimeout = null);
        xa(a.F) && (w.clearTimeout(a.F), a.F = null)
    }

    function Kv(a) {
        var b = a.Ea(),
            c;
        a: switch (b) {
            case 200:
            case 201:
            case 202:
            case 204:
            case 206:
            case 304:
            case 1223:
                c = !0;
                break a;
            default:
                c = !1
        }
        if (!c) {
            if (b = 0 === b) a = String(a.U).match(ir)[1] || null, !a && w.self && w.self.location && (a = w.self.location.protocol, a = a.substr(0, a.length - 1)), b = !zv.test(a ? a.toLowerCase() : "");
            c = b
        }
        return c
    }

    function Jv(a) {
        return a.A ? a.A.readyState : 0
    }
    q.Ea = function() {
        try {
            return 2 < Jv(this) ? this.A.status : -1
        } catch (a) {
            return -1
        }
    };

    function Lv(a) {
        try {
            if (!a.A) return null;
            if ("response" in a.A) return a.A.response;
            switch (a.D) {
                case "":
                case "text":
                    return a.A.responseText;
                case "arraybuffer":
                    if ("mozResponseArrayBuffer" in a.A) return a.A.mozResponseArrayBuffer
            }
            return null
        } catch (b) {
            return null
        }
    };

    function Mv() {
        this.A = 0;
        this.D = 2;
        this.B = 0;
        this.F = this.C = this.G = null
    }

    function Nv(a, b, c) {
        a.C = b;
        a.F = c || null
    }
    Mv.prototype.cancel = function() {
        if (3 == this.A) return !1;
        var a = !1;
        this.C && (a = this.C.call(this.F)) && (this.A = 3);
        return a
    };
    Mv.prototype.start = function(a) {
        if (0 != this.A) throw Error("Trying to reuse an Rpc object. Status is not INACTIVE");
        this.A = 1;
        this.G = a
    };
    Mv.prototype.done = aa();

    function Ov(a, b) {
        if (0 == b) throw Error("Trying to set the Rpc status to INACTIVE.");
        a.A = b
    }
    Mv.prototype.Ea = h("A");

    function Pv(a) {
        var b = new Mv;
        b.D = a.D;
        return b
    };

    function Qv(a, b) {
        this.B = a;
        this.C = b
    }
    Qv.prototype.A = function(a, b, c, d) {
        d = d || new Mv;
        a = new Rv(a, b, c ? c : null, d, this.B, this.C);
        Qr(this.B, a, d.D)
    };

    function Rv(a, b, c, d, e, f) {
        this.J = a;
        this.F = b;
        this.G = c;
        this.B = d;
        this.C = !1;
        this.A = null;
        this.K = e;
        this.I = f;
        this.D = !1;
        this.state = null;
        Nv(this.B, this.H, this)
    }
    Rv.prototype.start = function(a) {
        this.A = Pv(this.B);
        this.A.start(this.B.G + ".RequestSchedulerChannel");
        Ov(this.A, 1);
        var b = this;
        this.I.A(this.J, function(a) {
            b.D = !0;
            b.F(a);
            ++b.B.B
        }, function() {
            var c = b.G;
            b.A.done();
            Ov(b.B, b.A.Ea());
            c && c();
            a()
        }, this.A)
    };
    Rv.prototype.cancel = function() {
        return !this.A || this.D && !this.C ? !1 : this.A.cancel()
    };
    Rv.prototype.H = function() {
        this.C = !0;
        return this.K.remove(this)
    };

    function Sv(a) {
        a = (new lr(a)).toString();
        this.A = a += -1 == a.indexOf("?") ? "?" : "&"
    }

    function Tv(a, b) {
        if (0 == b.length) return a.A.slice(0, a.A.length - 1);
        if ("?" == b[0] || "&" == b[0]) b = b.slice(1);
        return a.A + b
    };

    function Uv(a, b, c) {
        this.B = wa(a) || a instanceof lr ? new Sv(a) : a;
        this.C = b;
        this.D = c || "GET"
    }

    function Vv(a, b, c, d) {
        function e(a) {
            el(c);
            3 != d.Ea() && a && b()
        }
        Vk(c, "success", function() {
            e(!0)
        });
        Vk(c, "abort", function() {
            e(!1)
        });
        Vk(c, "error", function() {
            Ov(d, 2);
            e(!0)
        });
        Vk(c, "timeout", function() {
            Ov(d, 2);
            e(!0)
        });
        Vk(c, "readystatechange", function() {
            var b = Lv(c);
            Kv(c) && 4 == Jv(c) && a(b)
        })
    }
    Uv.prototype.A = function(a, b, c, d) {
        d = d || new Mv;
        c = c || C;
        var e = new yv;
        e.O = !1;
        y(this.C) && (e.D = this.C);
        Nv(d, function() {
            e.abort();
            return !0
        });
        Vv(b, c, e, d);
        "POST" == this.D ? (b = Tv(this.B, ""), Bv(e, b, "POST", a, {
            "Content-type": "application/x-www-form-urlencoded"
        })) : (b = Tv(this.B, a), Bv(e, b))
    };

    function Wv(a, b, c) {
        this.B = a;
        this.C = b;
        this.D = c;
        this.A = C
    }

    function Xv(a, b, c, d) {
        b = a.C.A(b);
        var e = d || new Mv;
        e.start("GpmsConfigService.getConfig");
        a.B.A(b, function(b) {
            try {
                if (3 != e.Ea() && (++e.B, 1 == e.B)) {
                    var d = null;
                    try {
                        d = a.D.A(b)
                    } catch (k) {
                        Ov(e, 2), d = null
                    }
                    c(d)
                }
            } catch (k) {
                throw a.A(k), k;
            }
        }, function() {
            try {
                3 != e.Ea() && (0 == e.B && (Ov(e, 2), c(null)), e.done())
            } catch (f) {
                throw a.A(f), f;
            }
        }, e)
    };
    /*
     Portions of this code are from MochiKit, received by
     The Closure Authors under the MIT license. All other code is Copyright
     2005-2009 The Closure Authors. All Rights Reserved.
    */
    function Yv(a, b) {
        this.G = [];
        this.M = a;
        this.J = b || null;
        this.D = this.B = !1;
        this.C = void 0;
        this.K = this.L = this.I = !1;
        this.H = 0;
        this.A = null;
        this.F = 0
    }
    q = Yv.prototype;
    q.cancel = function(a) {
        if (this.B) this.C instanceof Yv && this.C.cancel();
        else {
            if (this.A) {
                var b = this.A;
                delete this.A;
                a ? b.cancel(a) : (b.F--, 0 >= b.F && b.cancel())
            }
            this.M ? this.M.call(this.J, this) : this.K = !0;
            this.B || this.Nd(new Zv)
        }
    };
    q.Uf = function(a, b) {
        this.I = !1;
        $v(this, a, b)
    };

    function $v(a, b, c) {
        a.B = !0;
        a.C = c;
        a.D = !b;
        aw(a)
    }

    function bw(a) {
        if (a.B) {
            if (!a.K) throw new cw;
            a.K = !1
        }
    }
    q.sa = function(a) {
        bw(this);
        $v(this, !0, a)
    };
    q.Nd = function(a) {
        bw(this);
        $v(this, !1, a)
    };

    function dw(a, b, c, d) {
        a.G.push([b, c, d]);
        a.B && aw(a)
    }
    q.then = function(a, b, c) {
        var d, e, f = new Sq(function(a, b) {
            d = a;
            e = b
        });
        dw(this, d, function(a) {
            a instanceof Zv ? f.cancel() : e(a)
        });
        return f.then(a, b, c)
    };
    Qq(Yv);
    Yv.prototype.la = function(a) {
        var b = new Yv;
        dw(this, b.sa, b.Nd, b);
        a && (b.A = this, this.F++);
        return b
    };

    function ew(a) {
        return pb(a.G, function(a) {
            return ya(a[1])
        })
    }

    function aw(a) {
        if (a.H && a.B && ew(a)) {
            var b = a.H,
                c = fw[b];
            c && (w.clearTimeout(c.ta), delete fw[b]);
            a.H = 0
        }
        a.A && (a.A.F--, delete a.A);
        for (var b = a.C, d = c = !1; a.G.length && !a.I;) {
            var e = a.G.shift(),
                f = e[0],
                g = e[1],
                e = e[2];
            if (f = a.D ? g : f) try {
                var k = f.call(e || a.J, b);
                y(k) && (a.D = a.D && (k == b || k instanceof Error), a.C = b = k);
                if (Rq(b) || "function" === typeof w.Promise && b instanceof w.Promise) d = !0, a.I = !0
            } catch (l) {
                b = l, a.D = !0, ew(a) || (c = !0)
            }
        }
        a.C = b;
        d && (k = E(a.Uf, a, !0), d = E(a.Uf, a, !1), b instanceof Yv ? (dw(b, k, d), b.L = !0) : b.then(k, d));
        c && (b = new gw(b),
            fw[b.ta] = b, a.H = b.ta)
    }

    function cw() {
        Ha.call(this)
    }
    H(cw, Ha);
    cw.prototype.message = "Deferred has already fired";
    cw.prototype.name = "AlreadyCalledError";

    function Zv() {
        Ha.call(this)
    }
    H(Zv, Ha);
    Zv.prototype.message = "Deferred was canceled";
    Zv.prototype.name = "CanceledError";

    function gw(a) {
        this.ta = w.setTimeout(E(this.B, this), 0);
        this.A = a
    }
    gw.prototype.B = function() {
        delete fw[this.ta];
        throw this.A;
    };
    var fw = {};

    function hw(a, b, c, d) {
        pv.call(this);
        this.K = new Rr(a);
        this.G = new lr(c);
        this.H = d
    }
    H(hw, pv);
    hw.prototype.D = function(a, b, c, d, e, f, g) {
        (a = this.Ha()) && 0 != N(a, 4) ? (a = new Pu(this.G.toString() + "/" + this.H), tu(a, b, Y.prototype.ng, Y.prototype.ih), tu(a, c, Y.prototype.og, Y.prototype.jh), tu(a, d, Y.prototype.Vg, Y.prototype.kh), b = a.Ic()) : b = this.G.toString() + "/s2560-no/" + this.H;
        e = f.sa(e, "fpts-get-tile");
        return this.K.Ja(b, e, g)
    };

    function iw(a, b, c, d, e, f) {
        b = new hw(c, 0, e, f);
        a(b)
    };

    function jw(a, b) {
        V.call(this, "HPNR", wb(arguments))
    }
    H(jw, V);

    function kw(a, b, c, d) {
        this.H = a;
        this.I = b;
        this.K = c;
        this.B = !1;
        this.N = -1;
        this.A = this.C = null;
        this.F = this.G = this.M = this.D = 0;
        this.J = !1;
        this.L = 0;
        this.P = !!d
    }

    function lw(a, b) {
        a.A.style.display = b ? "inline" : "none"
    }

    function mw(a) {
        a.A.style.left = a.D + "px";
        a.A.style.top = a.M + "px"
    }

    function nw(a) {
        a.A.style.width = a.G + "px";
        a.A.style.height = a.F + "px"
    }
    kw.prototype.O = function() {
        Rn(this.A, "background-image", "-webkit-linear-gradient(45deg,#efefef 25%,transparent 25%,transparent 75%,#efefef 75%,#efefef),-webkit-linear-gradient(45deg,#efefef 25%,transparent 25%,transparent 75%,#efefef 75%,#efefef)");
        Rn(this.A, "background-color", "#fff");
        Rn(this.A, "background-position", "0 0, 10px 10px");
        Rn(this.A, "-webkit-background-size", "21px 21px");
        Rn(this.A, "background-size", "21px 21px")
    };

    function ow(a, b, c) {
        a.C = b;
        b = b.Ja();
        a.A = c ? b.cloneNode(!0) : b;
        a.A.className = "tile-image-3d";
        a.J && (a.J = !1, a.L = F())
    }
    kw.prototype.remove = function() {
        this.A && this.A.parentElement && this.A.parentElement.removeChild(this.A)
    };

    function pw(a, b, c, d) {
        this.C = a;
        this.A = Rj("div");
        this.A.style.overflow = "hidden";
        this.A.style.position = "absolute";
        this.A.style.width = "inherit";
        this.A.style.height = "inherit";
        this.I = -1;
        this.F = b;
        this.M = c;
        this.K = d;
        var e = d.ma();
        a = e.D;
        d = a.W() * e.C;
        var f = Ce(a) * e.B,
            e = Math.pow(2, e.A - b);
        b = Math.pow(2, c - b);
        this.B = d / (e * a.W());
        this.D = f / (e * Ce(a));
        this.O = Math.ceil(this.B);
        this.N = Math.round(b * a.W());
        this.L = Math.round(b * Ce(a));
        this.J = {};
        this.H = [];
        this.G = 0
    }

    function qw(a, b) {
        b != a.I && (a.I = b, $n(a.A, b), 0 < b && a.A.parentElement != a.C ? a.C.appendChild(a.A) : 0 == b && a.A.parentElement && a.C.removeChild(a.A))
    };

    function rw(a) {
        this.D = a;
        this.A = null;
        this.H = ik();
        this.B = ik();
        this.I = [];
        this.F = Infinity;
        this.C = null;
        this.G = -1
    }
    var sw = rl(),
        tw = rl();

    function uw(a) {
        a.A || (a.A = Rj("div"), a.A.style.overflow = "hidden", a.A.style.position = "absolute", a.A.style.width = "inherit", a.A.style.height = "inherit");
        return a.A
    }

    function vw(a, b) {
        var c = Math.round(4 * b),
            d = a.I[c];
        d || (d = c / 4, b = Math.round(b), d = a.I[c] = new pw(uw(a), b, d, a.D));
        return d
    }
    rw.prototype.detach = function() {
        this.A && this.A.parentElement && this.A.parentElement.removeChild(this.A)
    };

    function ww(a, b) {
        il.call(this);
        this.B = new le;
        this.I = b;
        this.F = !1;
        this.G = new nl(5, function(a, b) {
            b.detach()
        });
        this.A = [];
        this.H = .5;
        this.C = function() {
            fp(a)
        };
        this.D = [];
        this.J = new Do(Mo())
    }
    H(ww, il);
    var xw = rl(),
        yw = El(),
        zw = new Dj;
    q = ww.prototype;
    q.xc = function(a) {
        this.B = a;
        this.C()
    };

    function Aw(a, b) {
        var c = pl(a.G, b.id());
        c || (c = new rw(b), ol(a.G, b.id(), c));
        return c
    }

    function Bw(a, b, c) {
        if (0 == a.A.length) return null;
        var d = a.A[0];
        a = Aw(a, d);
        sl(xw, b, c, 1);
        kk(a.B, xw, xw);
        xw[0] -= Math.floor(xw[0]);
        return d
    }
    q.Tc = function(a, b) {
        if (0 != this.A.length) {
            var c = this.A[0];
            if (0 != this.A.length) {
                var d = .5 * Ce(ue(this.B)),
                    e = .5 * ue(this.B).W(),
                    f = Aw(this, this.A[0]);
                sl(xw, e, d, 1);
                kk(f.B, xw, xw);
                this.H = xw[0]
            }
            d = this.H;
            e = c.ma();
            Fl(yw, e.O);
            $l(a, xw);
            Ol(yw, xw, xw);
            a = xw[0];
            e = xw[1];
            sl(xw, Math.atan2(a, e) / Math.PI * .5 + .5, -Math.atan2(xw[2], Math.sqrt(a * a + e * e)) / Math.PI + .5, 1);
            a = xw[0] - d + .5;
            a -= Math.floor(a);
            xw[0] = a - .5 + d;
            c = Aw(this, c);
            kk(c.H, xw, xw);
            b[0] = xw[0];
            b[1] = xw[1]
        }
    };
    q.Yd = function(a) {
        a[0] = 1;
        a[1] = 179;
        if (0 != this.A.length) {
            var b = this.B,
                b = Aw(this, this.A[0]).B[4] * Ce(ue(b)) * 90;
            a[0] = Math.max(27 + b, 1);
            a[1] = Math.min(156 - b, 179);
            a[0] > a[1] && (a[0] = (a[0] + a[1]) / 2, a[1] = a[0])
        }
    };
    q.Rd = function(a, b, c) {
        a = Bw(this, a, b);
        if (!a) return null;
        zw.x = xw[0];
        zw.y = xw[1];
        return a.Qd(zw, c)
    };
    q.Rc = function(a, b, c) {
        a = Bw(this, a, b);
        if (!a) return null;
        if (b = a.Pd()) {
            c = Gp(b, xw[0], xw[1], c);
            if (!c) return null;
            Np(a.ma(), yw);
            Ol(yw, c.origin, c.origin);
            Pl(yw, c.A, c.A);
            yl(c.A, c.A);
            return c
        }
        return null
    };
    q.hc = function(a, b) {
        if (!ap(this.A, a)) {
            Cw(this);
            for (var c = 0; c < this.A.length; ++c) {
                var d = this.A[c]; - 1 == mb(a, d) && Aw(this, d).detach()
            }
            this.A = [];
            for (c = 0; c < a.length; ++c) {
                var d = a[c],
                    e = zh(Ys(d.Da()));
                1 !== e && 2 !== e && 11 !== e && 13 !== e && 5 !== e && 4 !== e || $o(d) || (Zo(d) || (d.Rb(b), d.dc(this.C)), this.A.push(d))
            }
            for (a = 0; a < this.A.length; ++a) b = Qo(this.A[a], "TileReady", this.C), this.D.push(b)
        }
    };

    function Cw(a) {
        for (var b = 0; b < a.A.length; ++b) dl(a.D[b]);
        a.D = []
    }
    q.clear = function() {
        Cw(this);
        this.G.clear();
        this.A = []
    };
    q.yc = C;
    q.Zc = function(a) {
        Rn(this.I, "background-color", .5 < a ? "white" : "black")
    };
    q.Pa = function() {
        Dw(this, !0)
    };
    q.$b = function() {
        Dw(this, !1)
    };

    function Dw(a, b) {
        var c = a.F = !1,
            d = new wo(a.J, "render_html4_pano");
        d.Fd();
        for (var e = 0; e < a.A.length; ++e) {
            var f = a.A[e];
            if (Zo(f)) {
                var f = Aw(a, f),
                    g = a.B,
                    k = a.I,
                    l = d,
                    m = uw(f);
                m.parentElement != k && k.appendChild(m);
                var n = f.D.ma(),
                    p = ue(g),
                    m = p.W(),
                    r = Ce(p),
                    u = Ae(se(g)),
                    u = u - N(f.D.Ha(), 10),
                    t = Eb(N(se(g), 1), 0, 180),
                    k = n.A,
                    k = Math.round(4 * Eb(k - Math.log(Eb(pe(g), 0, 180) / 180 * Ce(n.D) * n.B / Ce(p)) / Math.LN2, 0, k)) / 4,
                    g = f.F;
                f.F = k;
                var v = f,
                    u = u / 360 + .5,
                    t = 1 - t / 180,
                    x = k,
                    D = n,
                    n = D.D,
                    z = n.W() * D.C,
                    A = Ce(n) * D.B,
                    n = p.W(),
                    p = Ce(p),
                    D = Math.pow(2, D.A - x),
                    x = D *
                    n / z,
                    z = D * p / A,
                    A = v.B,
                    p = z / p;
                A[0] = x / n;
                A[1] = 0;
                A[2] = 0;
                A[3] = 0;
                A[4] = p;
                A[5] = 0;
                A[6] = 0;
                A[7] = 0;
                A[8] = 1;
                v.B[6] = u - x / 2;
                v.B[7] = t - z / 2;
                v.B[8] = 1;
                jk(v.B, v.H);
                sl(sw, 0, 0, 1);
                sl(tw, m, r, 1);
                kk(f.B, sw, sw);
                kk(f.B, tw, tw);
                r = m = vw(f, k);
                n = sw;
                x = tw;
                ++r.G;
                v = n[1] * r.D;
                u = Math.min(r.D, x[1] * r.D);
                t = (n[0] - Math.floor(n[0])) * r.B;
                p = t + (x[0] - n[0]) * r.B;
                z = p + 1;
                r.K.ma();
                n = !1;
                A = r.F != r.M;
                x = r.H;
                r.H = [];
                for (D = Math.max(0, Math.floor(v)); D < u; ++D)
                    for (var B = (D - v) * r.L, Q = Math.floor(t); Q < z; ++Q) {
                        var M = Fb(Q, r.O),
                            G = Q < r.B ? Q : M + r.B;
                        if (!(G > p)) {
                            var G = (G - t) * r.N,
                                T = M + "|" + D +
                                "|" + r.F,
                                R = r.J[T];
                            R || (R = new kw(M, D, r.F), r.J[T] = R);
                            T = M = R;
                            R = B;
                            if (G != T.D || R != T.M) T.D = G, T.M = R, T.A && mw(T);
                            G = M;
                            T = r.N;
                            R = r.L;
                            if (T != G.G || R != G.F) G.G = T, G.F = R, G.A && nw(G);
                            G = M;
                            G.A && !0 !== G.B && lw(G, !0);
                            G.B = !0;
                            var G = M,
                                T = r.K,
                                R = r.A,
                                qa = l,
                                na = A;
                            if (G.C && !na) {
                                var sa = T.tb(G.H, G.I, G.K);
                                sa && G.C != sa && (G.remove(), ow(G, sa, na))
                            }
                            G.C || ((sa = T.tb(G.H, G.I, G.K)) ? ow(G, sa, na) : (T.Gb(G.H, G.I, G.K, qa), G.J = !0));
                            G.A && G.A.parentElement != R && (R.appendChild(G.A), G.A.style.position = "absolute", G.A.style.pointerEvents = "none", lw(G, G.B), mw(G), nw(G), G.P &&
                                (G.A.complete || "complete" == G.A.readyState ? G.O() : bl(G.A, ["load", "readystatechange"], E(G.O, G))));
                            G = M;
                            G.A ? (T = Eb((F() - G.L) / 250, 0, 1), $n(G.A, T), G = 1 > T) : G = !1;
                            n = G || n;
                            M.N = r.G;
                            r.H.push(M)
                        }
                    }
                for (l = 0; l < x.length; ++l) M = x[l], M.N != r.G && (v = M, v.A && !1 !== v.B && lw(v, !1), v.B = !1);
                r = n;
                Infinity != g && k != g && (k = vw(f, g), f.C && f.C.M != g && qw(f.C, 0), f.G = F(), f.C = k, f.C.A.style.zIndex = 1, m.A.style.zIndex = 0);
                f.C && (g = (F() - f.G) / 250, g = Eb(g, 0, 1), qw(f.C, 1 - g), 1 > g ? r = !0 : f.C = null);
                qw(m, 1);
                c = r || c
            }
        }
        c ? b && a.C() : (a.F = !0, a.dispatchEvent(new Ek("ViewportReady",
            a)));
        d.done("main-actionflow-branch")
    }
    q.Yc = h("F");

    function Ew(a, b, c, d) {
        b = new ww(c, d);
        a(b)
    };

    function Fw(a, b, c, d, e, f, g) {
        V.call(this, "GCS", wb(arguments))
    }
    H(Fw, V);

    function Gw(a) {
        this.D = null;
        this.J = this.B = this.G = this.H = 0;
        this.C = [];
        this.I = [];
        this.F = {};
        this.K = {};
        this.A = new Xr(a)
    }
    var Hw = new Float32Array(3),
        Iw = ok(),
        Jw = new zm;
    q = Gw.prototype;
    q.$d = ba("D");
    q.Zd = ca(0);
    q.start = function() {
        return this.D ? this.hl : rj
    };
    q.hl = function() {
        if (8 != Kp(this.A, 0) || 8 != Kp(this.A, 7)) return rj;
        this.B = (bs(this.A, 1) || 1) - 1;
        this.H = bs(this.A, 3) || 0;
        this.G = bs(this.A, 5) || 0;
        var a = Kp(this.A, 7) || 0;
        this.J = a;
        this.C = Array(this.B + 1);
        var a = a + this.H * this.G,
            b = 22 * this.B,
            c;
        c = this.A;
        c = !c.A || 0 > a ? [] : as(c, a, b) || [];
        this.C[0] = "";
        for (var d = 1; d <= this.B; d++) {
            for (var e = "", f = 0; 22 > f; f++) e += String.fromCharCode(c[22 * (d - 1) + f] || 0);
            this.C[d] = e
        }
        a = cs(this.A, a + b, 2 * this.B);
        if (a.length != 2 * this.B) return this.Ig;
        this.I = a;
        return this.zi
    };
    q.zi = function() {
        var a = this.D.Ha(),
            b = qe(a.fa()),
            c = cm(xe(b));
        sk(Iw, -Jb(N(a, 10)));
        for (b = 0; b < this.B; b++) {
            var d = this.C[b + 1],
                e = this.I[2 * b + 1],
                f = Hw;
            f[0] = this.I[2 * b];
            f[1] = e;
            f[2] = 0;
            var e = Iw,
                g = Hw,
                f = Hw,
                k = g[0],
                l = g[1],
                g = g[2];
            f[0] = k * e[0] + l * e[4] + g * e[8] + e[12];
            f[1] = k * e[1] + l * e[5] + g * e[9] + e[13];
            f[2] = k * e[2] + l * e[6] + g * e[10] + e[14];
            km(a.fa(), Jw);
            e = nm(Jw);
            e.A += Hw[0] * c;
            e.B += Hw[1] * c;
            lm(Jw, e);
            e = new le;
            mm(Jw, e);
            this.F[d] = qe(e);
            this.K[d] = 2
        }
        for (b = 0; b < S(a, 19); ++b) c = new wh(P(new Qs(Nc(a, 19, b)), 4)), d = Ns(c), (d = this.F[d]) && U(re(Bh(c)),
            d);
        return this.Ig
    };
    q.Ig = function() {
        var a = this.D.Cb(),
            b = this.G,
            c = this.A,
            d = this.J,
            e = this.C,
            f = this.F,
            g = this.K;
        a.A = this.H;
        a.C = b;
        a.B = c;
        a.D = d;
        a.G = e;
        a.H = f;
        a.F = g;
        return rj
    };

    function Kw(a, b, c, d) {
        this.B = null;
        this.F = a;
        this.D = b;
        this.A = [];
        for (a = 0; a < S(c, 0); ++a) {
            b = 4 * a;
            var e = new Og((new Ng(Nc(c, 0, a))).data[0]);
            this.A[b + 0] = -N(e, 0);
            this.A[b + 1] = -N(e, 1);
            this.A[b + 2] = -N(e, 2);
            this.A[b + 3] = -N(e, 3)
        }
        this.C = new Xr(d)
    }
    Kw.prototype.$d = ba("B");
    Kw.prototype.Zd = ca(1);
    Kw.prototype.start = function() {
        return this.B ? this.G : rj
    };
    Kw.prototype.G = function() {
        var a = this.B;
        a.C = new zp(this.F, this.D, this.A, this.C);
        a.Jb();
        return rj
    };

    function Lw(a, b, c, d) {
        this.A = null;
        this.H = a;
        this.G = b;
        this.F = new Xr(d);
        this.C = [];
        this.B = {};
        this.D = {};
        for (a = 0; a < S(c, 0); ++a) {
            b = Ug(Vg(c, a)).za();
            this.C.push(b);
            this.B[b] = L(Ug(Vg(c, a)), 0);
            d = new Te(Vg(c, a).data[2]);
            var e = new me;
            e.data[1] = N($e(d), 3);
            e.data[2] = N($e(d), 2);
            ze(e, N(new We(d.data[1]), 1) || 3);
            this.D[b] = e
        }
    }
    Lw.prototype.$d = ba("A");
    Lw.prototype.Zd = ca(0);
    Lw.prototype.start = function() {
        return this.A ? this.I : rj
    };
    Lw.prototype.I = function() {
        var a = this.A.Cb(),
            b = this.G,
            c = this.F,
            d = this.C,
            e = this.D,
            f = this.B;
        a.A = this.H;
        a.C = b;
        a.B = c;
        a.D = 0;
        a.G = d;
        a.H = e;
        a.F = f;
        return rj
    };

    function Mw(a, b) {
        var c = b || {};
        b = c.document || document;
        var d = Bi(a),
            e = Rj("SCRIPT");
        a = {
            ph: e,
            Hb: void 0
        };
        var f = new Yv(Nw, a),
            g = null,
            k = null != c.timeout ? c.timeout : 5E3;
        0 < k && (g = window.setTimeout(function() {
            Ow(e, !0);
            f.Nd(new Pw(1, "Timeout reached for loading script " + d))
        }, k), a.Hb = g);
        e.onload = e.onreadystatechange = function() {
            e.readyState && "loaded" != e.readyState && "complete" != e.readyState || (Ow(e, c.xi || !1, g), f.sa(null))
        };
        e.onerror = function() {
            Ow(e, !0, g);
            f.Nd(new Pw(0, "Error while loading script " + d))
        };
        a = c.attributes || {};
        Sb(a, {
            type: "text/javascript",
            charset: "UTF-8",
            src: d
        });
        Lj(e, a);
        Qw(b).appendChild(e);
        return f
    }

    function Qw(a) {
        var b;
        return (b = (a || document).getElementsByTagName("HEAD")) && 0 != b.length ? b[0] : a.documentElement
    }

    function Nw() {
        if (this && this.ph) {
            var a = this.ph;
            a && "SCRIPT" == a.tagName && Ow(a, !0, this.Hb)
        }
    }

    function Ow(a, b, c) {
        null != c && w.clearTimeout(c);
        a.onload = C;
        a.onerror = C;
        a.onreadystatechange = C;
        b && window.setTimeout(function() {
            Yj(a)
        }, 0)
    }

    function Pw(a, b) {
        a = "Jsloader error (code #" + a + ")";
        b && (a += ": " + b);
        Ha.call(this, a)
    }
    H(Pw, Ha);

    function Rw(a, b) {
        this.B = new lr(a);
        this.A = b ? b : "callback";
        this.Hb = 5E3
    }
    var Sw = 0;

    function Tw(a, b, c) {
        var d = "_" + (Sw++).toString(36) + F().toString(36),
            e = "_callbacks___" + d,
            f = new lr(a.B);
        b && (w[e] = Uw(d, b), b = a.A, ua(e) || (e = [String(e)]), Cr(f.A, b, e));
        a = {
            timeout: a.Hb,
            xi: !0
        };
        e = new zi;
        e.A = f.toString();
        f = Mw(e, a);
        dw(f, null, Vw(d, c), void 0);
        return {
            ta: d,
            Xf: f
        }
    }
    Rw.prototype.cancel = function(a) {
        a && (a.Xf && a.Xf.cancel(), a.ta && Ww(a.ta, !1))
    };

    function Vw(a, b) {
        return function() {
            Ww(a, !1);
            b && b(null)
        }
    }

    function Uw(a, b) {
        return function(c) {
            Ww(a, !0);
            b.apply(void 0, arguments)
        }
    }

    function Ww(a, b) {
        a = "_callbacks___" + a;
        if (w[a])
            if (b) try {
                delete w[a]
            } catch (c) {
                w[a] = void 0
            } else w[a] = C
    };

    function Xw(a) {
        this.B = wa(a) || a instanceof lr ? new Sv(a) : a
    }
    Xw.prototype.A = function(a, b, c, d) {
        var e = d || new Mv,
            f = c || C,
            g = new Rw(Tv(this.B, a)),
            k = !1,
            l = Tw(g, function(a) {
                Ov(e, 1);
                b(a);
                f()
            }, function() {
                k || (Ov(e, 2), f())
            });
        Nv(e, function() {
            k = !0;
            var a;
            null === l ? a = !1 : (g.cancel(l), a = !0);
            return a
        })
    };

    function Yw(a, b) {
        var c = Zw(a, b),
            c = Array(c);
        $w(a, b, c, 0);
        return c.join("")
    }
    var ax = /^([0-9]+)([a-z])([\s\S]*)/,
        bx = /(\*)/g,
        cx = /(!)/g,
        dx = /(\*2A)/gi,
        ex = /(\*21)/gi;

    function Zw(a, b) {
        var c = 0,
            d;
        for (d in b.R) {
            var e = parseInt(d, 10),
                f = b.R[e],
                e = a[e + b.A];
            if (f && null != e)
                if (3 == f.label)
                    for (var g = 0; g < e.length; ++g) c += fx(e[g], f);
                else c += fx(e, f)
        }
        return c
    }

    function fx(a, b) {
        var c = 4;
        "m" == b.type && (c += Zw(a, b.Ce));
        return c
    }

    function $w(a, b, c, d) {
        for (var e in b.R) {
            var f = parseInt(e, 10),
                g = b.R[f],
                k = a[f + b.A];
            if (g && null != k)
                if (3 == g.label)
                    for (var l = 0; l < k.length; ++l) d = gx(k[l], f, g, c, d);
                else d = gx(k, f, g, c, d)
        }
        return d
    }

    function gx(a, b, c, d, e) {
        d[e++] = "!";
        d[e++] = "" + b;
        if ("m" == c.type) d[e++] = c.type, d[e++] = "", b = e, e = $w(a, c.Ce, d, e), d[b - 1] = "" + (e - b >> 2);
        else {
            c = c.type;
            if ("b" == c) a = a ? "1" : "0";
            else if ("i" == c || "j" == c || "u" == c || "v" == c || "n" == c || "o" == c) {
                if (!wa(a) || "j" != c && "v" != c && "o" != c) a = "" + Math.floor(a)
            } else if (a = "" + a, "s" == c) {
                var f = a;
                b = encodeURIComponent(f).replace(/%20/g, "+");
                var g = b.match(/%[89AB]/ig),
                    f = f.length + (g ? g.length : 0);
                if (4 * Math.ceil(f / 3) - (3 - f % 3) % 3 < b.length) {
                    c = a;
                    a = [];
                    for (f = b = 0; f < c.length; f++) g = c.charCodeAt(f), 128 > g ? a[b++] =
                        g : (2048 > g ? a[b++] = g >> 6 | 192 : (55296 == (g & 64512) && f + 1 < c.length && 56320 == (c.charCodeAt(f + 1) & 64512) ? (g = 65536 + ((g & 1023) << 10) + (c.charCodeAt(++f) & 1023), a[b++] = g >> 18 | 240, a[b++] = g >> 12 & 63 | 128) : a[b++] = g >> 12 | 224, a[b++] = g >> 6 & 63 | 128), a[b++] = g & 63 | 128);
                    va(a);
                    ae();
                    c = Yd;
                    b = [];
                    for (f = 0; f < a.length; f += 3) {
                        var k = a[f],
                            l = (g = f + 1 < a.length) ? a[f + 1] : 0,
                            m = f + 2 < a.length,
                            n = m ? a[f + 2] : 0,
                            p = k >> 2,
                            k = (k & 3) << 4 | l >> 4,
                            l = (l & 15) << 2 | n >> 6,
                            n = n & 63;
                        m || (n = 64, g || (l = 64));
                        b.push(c[p], c[k], c[l], c[n])
                    }
                    a = b.join("");
                    a = a.replace(/\.+$/, "");
                    c = "z"
                } else -1 != a.indexOf("*") &&
                    (a = a.replace(bx, "*2A")), -1 != a.indexOf("!") && (a = a.replace(cx, "*21"))
            }
            d[e++] = c;
            d[e++] = a
        }
        return e
    }

    function Wf(a) {
        return -1 != a.indexOf("*21") ? a.replace(ex, "!") : a
    }

    function Xf(a) {
        var b = a.charCodeAt(0).toString(16),
            c = new RegExp("(\\*" + b + ")", "gi"),
            b = "*" + b,
            d = b.toLowerCase();
        return function(e) {
            return -1 != e.indexOf(b) || -1 != e.indexOf(d) ? e.replace(c, a) : e
        }
    }

    function Yf(a, b, c, d, e, f) {
        if (a + b > c.length) return !1;
        var g = a;
        for (a += b; g < a; ++g) {
            var k = ax.exec(c[g]);
            if (!k) return !1;
            b = parseInt(k[1], 10);
            var l = k[2],
                m = k[3],
                m = d(m);
            if (-1 != m.indexOf("*2A") || -1 != m.indexOf("*2a")) m = m.replace(dx, "*");
            var n = 0;
            if ("m" == l && (n = parseInt(m, 10), isNaN(n))) return !1;
            var p = e.R[b];
            if (p) {
                k = k[2];
                if ("z" == k) {
                    for (var k = "s", m = Zd(m), l = [], r = 0, u = 0; r < m.length;) {
                        var t = m[r++];
                        if (128 > t) l[u++] = String.fromCharCode(t);
                        else if (191 < t && 224 > t) {
                            var v = m[r++];
                            l[u++] = String.fromCharCode((t & 31) << 6 | v & 63)
                        } else if (239 <
                            t && 365 > t) {
                            var v = m[r++],
                                x = m[r++],
                                D = m[r++],
                                t = ((t & 7) << 18 | (v & 63) << 12 | (x & 63) << 6 | D & 63) - 65536;
                            l[u++] = String.fromCharCode(55296 + (t >> 10));
                            l[u++] = String.fromCharCode(56320 + (t & 1023))
                        } else v = m[r++], x = m[r++], l[u++] = String.fromCharCode((t & 15) << 12 | (v & 63) << 6 | x & 63)
                    }
                    m = l.join("")
                }
                if (p.type != k) return !1;
                if ("m" == p.type) {
                    p = p.Ce;
                    m = [];
                    if (!Yf(g + 1, n, c, d, p, m)) return !1;
                    g += n
                }
                a: {
                    n = m;p = f;k = e.R[b];
                    if ("s" != k.type && "m" != k.type && !wa(k.Bi)) {
                        m = "f" != k.type && "d" != k.type ? parseInt(n, 10) : parseFloat(n);
                        if (isNaN(m)) {
                            b = !1;
                            break a
                        }
                        "b" == k.type ?
                            n = 0 != m : n = m
                    }
                    b += e.A || 0;3 == k.label ? cc(p, b).push(n) : p[b] = n;b = !0
                }
                if (!b) return !1
            } else "m" == l && (g += n)
        }
        return !0
    };

    function hx(a) {
        this.B = a
    }
    hx.prototype.A = function(a) {
        a = Yw(a.ib(), this.B);
        return "pb=" + encodeURIComponent(a).replace(/%20/g, "+")
    };

    function ix(a) {
        this.B = a
    }
    ix.prototype.A = function(a) {
        return new this.B(a)
    };

    function jx(a, b) {
        this.C = a;
        this.B = b || C
    }
    jx.prototype.A = function(a) {
        ")]}'\n" == a.substr(0, 5) && (a = a.substr(5));
        var b, c = !1;
        if (w.JSON && w.JSON.parse && 0 > a.indexOf(",,") && 0 > a.indexOf("[,") && 0 > a.indexOf(",]")) try {
            b = JSON.parse(a), c = !0
        } catch (d) {}
        if (!c) try {
            b = $t(a)
        } catch (d) {}
        if (!(b instanceof Array)) throw this.B(a), Error("JspbDeserializer parse error.");
        return new this.C(b)
    };

    function kx(a, b, c, d, e, f, g) {
        this.F = c ? new Wv(new Qv(d, new Xw(a)), new hx(nh()), new ix(Zg)) : new Wv(new Qv(d, new Uv(a)), new hx(nh()), new jx(Zg, qj));
        this.C = b ? c ? new Wv(new Qv(d, new Xw(b)), new hx(vh()), new ix(jh)) : new Wv(new Qv(d, new Uv(b)), new hx(vh()), new jx(jh, qj)) : null;
        this.D = f;
        this.B = e;
        this.A = g
    }
    kx.prototype.Ha = function(a, b, c) {
        lx(this, a.Da(), c.sa(b, "gcs-get-config"))
    };

    function mx(a, b, c) {
        var d = Ys(b),
            e = new hh;
        vf(new sf(P(e, 0)), a.B);
        var f = new Pf;
        Vf(f, d.za());
        K(f, 0) ? U(new Pf(P(e, 6)), f) : (f = new $f(P(e, 1)), b = qe(Ys(b).fa()), bf(new Ue(P(f, 0)), xe(b)), cf(new Ue(P(f, 0)), we(b)), f.data[1] = 50);
        U(new od(P(new jg(P(e, 2)), 1)), a.D);
        Kc(new Rd(P(new Pd(P(new jg(P(e, 2)), 0)), 0)), 0, 2);
        (new Pd(P(new jg(P(e, 2)), 0))).data[2] = !0;
        b = new kf(Mc(new Cf(P(new jg(P(e, 2)), 10)), 0));
        b.data[0] = 2;
        b.data[1] = !0;
        b.data[2] = 2;
        a.A && U(new vd(P(new sf(P(e, 0)), 10)), a.A);
        nx(new bh(P(e, 3)));
        var g = new Mv;
        Xv(a.C,
            e,
            function(a) {
                a && K(a, 1) ? ox(a.getMetadata(), c, g, d) : c(new es)
            }, g)
    }

    function lx(a, b, c) {
        var d = Ys(b),
            e = new Xg,
            f = new $g(Mc(e, 2));
        if (K(d.ia(), 1)) U(new Ne(P(f, 0)), ph(d.ia()));
        else {
            Re(new Ne(P(f, 0)), Ns(d));
            var g = 0;
            switch (L(Ys(b), 1, 99)) {
                case 7:
                    g = 1;
                    break;
                case 0:
                    g = 2;
                    break;
                case 4:
                    g = 3;
                    break;
                case 1:
                    g = 4;
                    break;
                default:
                    g = px(zh(d))
            }
            Qe(new Ne(P(f, 0)), g)
        }
        nx(new bh(P(e, 3)));
        f = null;
        0 < S(d, 15) && 0 == L(new Af(Oh(d).data[0]), 8) && (f = O(new Af(Oh(d).data[0]), 0), Nf(new Jf(P(new $g(Nc(e, 2, 0)), 1)), f));
        U(new od(P(e, 1)), a.D);
        a.A && U(new vd(P(new sf(P(e, 0)), 10)), a.A);
        vf(new sf(P(e, 0)), a.B);
        var k = new Mv;
        Xv(a.F, e, function(e) {
            1 === k.Ea() && e && S(e, 1) && 0 === e.Ea().rc() && (1 === e.eb(0).Ea().rc() || 3 === e.eb(0).Ea().rc()) ? ox(e.eb(0), c, k, d) : a.C ? mx(a, b, c) : c(new es)
        }, k)
    }

    function nx(a) {
        Kc(a, 0, 1);
        Kc(a, 0, 2);
        Kc(a, 0, 3);
        Kc(a, 0, 4);
        Kc(a, 0, 5);
        Kc(a, 0, 6);
        Kc(a, 0, 8);
        (new Cg(Mc(a, 1))).data[0] = 1;
        (new dh(P(a, 3))).data[0] = 48;
        (new Eg(Mc(a, 5))).data[0] = 1;
        (new Eg(Mc(a, 5))).data[0] = 2;
        (new Lg(Mc(a, 4))).data[0] = 1;
        (new Lg(Mc(a, 4))).data[0] = 2
    }

    function qx(a) {
        for (var b = 0; b < S(a, 5); ++b) {
            var c = uh(a, b);
            if (K(c, 5)) {
                var d = new Rg(c.data[5]);
                if (K(d, 1)) return a = new Qg(d.data[1]), b = O(a, 2), 2 == L(new Lg(d.data[0]), 0) ? new ds(b) : new Kw((new ld(a.data[0])).W(), N(new ld(a.data[0]), 0), new Pg(c.data[4]), b)
            }
        }
        return new ds("")
    }

    function ox(a, b, c, d) {
        var e = new es;
        if (1 === c.Ea() && (1 === a.Ea().rc() || 3 === a.Ea().rc()))
            if (e.A = a, Es(d)) {
                e.C = dt(a);
                e.B.push(qx(a));
                a: {
                    for (var f = 0; f < S(a, 5); ++f)
                        if (c = uh(a, f), K(c, 5) && (d = new Rg(c.data[5]), K(d, 3))) {
                            a = new Qg(d.data[3]);
                            f = String(O(a, 2));
                            a = 2 == L(new Eg(d.data[2]), 0) ? new Gw(f) : new Lw((new ld(a.data[0])).W(), N(new ld(a.data[0]), 0), new Kg(c.data[3]), f);
                            break a
                        }
                    a = null
                }
                a && e.B.push(a)
            } else {
                c = new at;
                d = null;
                for (f = 0; f < S(a, 5); ++f) {
                    var g = uh(a, f);
                    if (K(g, 1) && (d = g, 2 === L(new Cg(d.data[0]), 0))) break
                }
                d && (f = new Bf,
                    U(new Te(P(f, 0)), Tg(d)), Js(f, new le(P(new ct(P(c, 1)), 0))));
                K(qh(a), 2) && (d = new ld(qh(a).data[2]), f = new le(P(new ct(P(c, 1)), 0)), Be(ve(f), d.W()), De(ve(f), N(d, 0)));
                d = new bt(P(c, 2));
                f = L(qh(a), 0);
                4 == f && (d.data[0] = 1);
                3 == f && (d.data[0] = 2);
                K(a, 4) && (g = new Ie(a.data[4]), 0 < S(g, 1) && (g = ge(Ke(new Je(Nc(g, 1, 0)))), d.data[3] = g));
                d.data[4] = K(sh(a), 5) ? ge(new de(sh(a).data[5])) : ge(new de(sh(a).data[6]));
                g = ph(a).za();
                d.data[1] = g;
                K(qh(a), 3) && 2 != f && (a = jf(qh(a)), d = new oe(P(c, 8)), K(a, 1) ? (Be(d, (new ld(a.data[1])).W()), De(d,
                    N(new ld(a.data[1]), 0))) : (Be(d, (new ld((new ff(Nc(a, 0, 0))).data[0])).W()), De(d, N(new ld((new ff(Nc(a, 0, 0))).data[0]), 0))), c.data[9] = S(a, 0) - 1);
                e.C = c
            }
        b(e)
    }

    function px(a) {
        switch (a) {
            case 1:
            case 2:
            case 4:
            case 5:
            case 11:
            case 13:
            case 3:
                return 2;
            case 10:
                return 4;
            case 12:
            case 15:
                return 3;
            case 27:
                return 1;
            default:
                return 0
        }
    };

    function rx(a, b, c, d, e, f, g, k, l) {
        b = new kx(c, d, e, f, g, k, l);
        a(b)
    };

    function sx(a, b, c, d, e) {
        V.call(this, "SCPI", wb(arguments))
    }
    H(sx, V);

    function tx(a, b) {
        this.C = a;
        this.A = b
    }
    tx.prototype.B = function(a) {
        var b = this.C;
        a = Gb(.2, 1, this.A ? a : 1 - a);
        b.F && b.B.B() && (b.B.A().I(a), fp(b.G))
    };

    function ux(a, b) {
        return vx(a.ib(), b.ib())
    }

    function vx(a, b) {
        return Cb(a, b, function(a, b) {
            return a instanceof Array && b instanceof Array ? vx(a, b) : a === b
        })
    }

    function wx(a, b) {
        return xx(a) && Xs(a) == Xs(b) ? K(a, 4) && K(b, 4) ? Ms(Ys(a), Ys(b)) : ux(a, b) : !1
    }

    function yx(a) {
        var b = new Vs;
        U(b, a);
        return b
    }

    function zx(a) {
        a = Xs(a);
        return 0 === a || 3 === a
    }

    function xx(a) {
        a = Xs(a);
        return 2 === a || 1 === a || 4 === a
    }

    function Ax(a, b) {
        var c = !1;
        K(a, 0) && (b.data[0] = Xs(a), c = !0);
        K(a, 4) && (U(Zs(b), Ys(a)), c = !0);
        K(a, 2) && (U(new Ws(P(b, 2)), new Ws(a.data[2])), c = !0);
        K(a, 3) && (U(new Ss(P(b, 3)), new Ss(a.data[3])), c = !0);
        K(a, 5) && (b.data[5] = O(a, 5), c = !0);
        K(a, 6) && (b.data[6] = Ic(a, 6), c = !0);
        K(a, 8) && (U(new Ts(P(b, 8)), $s(a)), c = !0);
        return c
    }

    function Bx(a) {
        var b = new Vs;
        U(Zs(b), a);
        b.data[0] = Es(a) ? 1 : Fs(a) ? 2 : 4;
        if (!K(a, 1)) {
            var c = Cx(a);
            Zs(b).data[1] = c
        }
        if (K(a, 21)) {
            a: {
                a = a.ia();
                for (c = 0; c < S(a, 5); ++c)
                    if (K(uh(a, c), 1)) {
                        var d = $e(Tg(uh(a, c)));
                        if (K(d, 4)) {
                            a = Dx[L(d, 4, 1)];
                            break a
                        }
                    }
                a = 0
            }
            0 != a && (b.data[7] = a)
        }
        return b
    }

    function Cx(a) {
        var b = qh(a.ia());
        if (K(b, 0)) switch (L(b, 0)) {
            case 1:
                return 7;
            case 2:
                return 0;
            case 3:
            case 8:
                return 4;
            case 4:
                return 1
        }
        b = 99;
        switch (zh(a)) {
            case 1:
            case 2:
            case 4:
            case 5:
            case 11:
            case 13:
            case 3:
                b = 0;
                break;
            case 10:
                b = 1;
                break;
            case 12:
            case 15:
                b = 4;
                break;
            case 7:
            case 14:
                b = 5;
                break;
            case 27:
                b = 7
        }
        return b
    }
    var Dx = {
        1: 1,
        2: 2,
        3: 3
    };

    function Ex(a) {
        this.A = y(a) ? a : 1;
        this.B = !0;
        this.C = !1
    }
    Ex.prototype.D = function() {
        var a = new Ex;
        a.A = this.A;
        a.B = this.B;
        a.C = this.C;
        return a
    };

    function Fx(a, b, c, d) {
        zs(a);
        this.B = zs(c);
        yx(b);
        this.A = yx(d)
    };

    function Gx(a) {
        for (; - 180 > a;) a += 360;
        for (; 180 < a;) a -= 360;
        return a
    };

    function Hx(a, b, c, d, e, f, g) {
        this.L = zs(a);
        this.P = b;
        this.N = Ys(this.P);
        this.J = zs(c);
        this.M = d;
        this.B = Ys(this.M);
        this.Y = e;
        this.C = f;
        this.F = zs(a);
        this.I = new Vo;
        U(re(new le(P(this.I, 2))), qe(a));
        this.I.data[5] = !0;
        this.A = new Vo;
        U(re(new le(P(this.A, 2))), e ? qe(a) : qe(c));
        this.A.data[5] = !0;
        this.U = null;
        this.X = !1;
        this.O = 0;
        this.K = g
    }
    Hx.prototype.D = ca(500);
    Hx.prototype.G = function(a) {
        Ix(this);
        var b;
        b = 2 * a - 1;
        b = .5 * (b * (2 + b * ((0 <= b ? -1 : 1) + 0 * b)) + 1);
        if (!this.Y) {
            var c = qe(this.L),
                d = qe(this.J),
                e = re(this.F),
                f = we(c);
            e.data[1] = Gx(Gb(f, f + Lb(f, we(d)), b));
            e.data[2] = Gb(xe(c), xe(d), b);
            ze(e, Gb(ye(c), ye(d), b))
        }
        var c = se(this.L),
            d = se(this.J),
            e = te(this.F),
            f = Ae(c),
            g = N(c, 2),
            k = Lb(g, N(d, 2));
        e.data[0] = Ib(f + Gb(0, Lb(f, Ae(d)), b));
        e.data[1] = Gb(N(c, 1), N(d, 1), b);
        e.data[2] = Ib(g + Gb(0, k, b));
        c = pe(this.L);
        d = pe(this.J);
        this.F.data[3] = 1E-6 >= Math.abs(c - d) ? c : Gb(c, d, b);
        0 === a || Ms(this.N, this.B) ?
            Jx(this.C, this.N, this.K) : 1 === a ? (Jx(this.C, this.B, this.K), K(this.B.fa(), 0) && U(re(this.F), qe(this.B.fa()))) : (this.I.data[0] = 1, !this.X || .2 > a ? (this.O = a, this.A.data[0] = 0) : this.A.data[0] = (a - this.O) / (1 - this.O), this.I.data[4] = Gb(1, 0, 10 * Eb(b, 0, .1)), a = Gb(0, 1, 10 * (Eb(b, .9, 1) - .9)), this.A && (this.A.data[4] = a), a = this.C, f = this.B, b = this.K, c = this.I, d = this.A, e = Kx(a, this.N, b), f = Kx(a, f, b), e && f && (Lx(a, e, b, c), a.L = f, a.K = d, Mx(a, b)));
        return this.F
    };
    Hx.prototype.H = function(a) {
        var b = !Ms(Ys(this.P), Ys(this.M));
        if (0 === a) b && (a = this.C, Nx(a.A, !1), a.B.B() && a.B.A().B(!1));
        else if (1 === a) return b && (a = this.C, Nx(a.A, !0), a.B.B() && a.B.A().B(!0)), this.M;
        return null
    };

    function Ix(a) {
        if (!a.U) {
            var b = Bx(a.B);
            a.U = Ox(a.C, b, a.K, function(b) {
                a.X = b
            })
        }
    };

    function Px(a, b, c) {
        this.K = Ae(se(a));
        this.C = zs(a);
        this.J = b ? -1 : 1;
        this.I = c;
        this.B = -2 / 3 * .075;
        this.F = 1 / .016875;
        this.A = 1 / (1 + this.B)
    }
    Px.prototype.D = ca(4E3);
    Px.prototype.G = function(a) {
        this.I && (a = .075 >= a ? this.A * this.F * a * a * a : this.A * (a + this.B));
        a = Ib(this.K + 360 * this.J * a);
        te(this.C).data[0] = a;
        return this.C
    };
    Px.prototype.H = ca(null);

    function Qx(a, b, c) {
        this.A = a;
        this.C = b;
        this.B = c
    }

    function Rx(a, b) {
        if (0 == b) return 0;
        if (1 == b) return 1;
        var c = Gb(0, a.A, b),
            d = Gb(a.A, a.B, b);
        a = Gb(a.B, 1, b);
        c = Gb(c, d, b);
        d = Gb(d, a, b);
        return Gb(c, d, b)
    }

    function Sx(a, b) {
        if (0 == b) return 0;
        if (1 == b) return 1;
        var c = Gb(0, a.C, b);
        a = Gb(a.C, 1, b);
        var d = Gb(1, 1, b),
            c = Gb(c, a, b);
        a = Gb(a, d, b);
        return Gb(c, a, b)
    }

    function Tx(a, b) {
        var c = (b - 0) / 1;
        if (0 >= c) return 0;
        if (1 <= c) return 1;
        for (var d = 0, e = 1, f = 0, g = 0; 8 > g; g++) {
            var f = Rx(a, c),
                k = (Rx(a, c + 1E-6) - f) / 1E-6;
            if (1E-6 > Math.abs(f - b)) return c;
            if (1E-6 > Math.abs(k)) break;
            else f < b ? d = c : e = c, c -= (f - b) / k
        }
        for (g = 0; 1E-6 < Math.abs(f - b) && 8 > g; g++) f < b ? (d = c, c = (c + e) / 2) : (e = c, c = (c + d) / 2), f = Rx(a, c);
        return c
    };

    function Ux(a, b, c) {
        a = new Qx(a, b, c);
        var d = Array(51);
        for (b = 0; 51 > b; b++) d[b] = Sx(a, Tx(a, b / 50));
        return function(a) {
            if (0 >= a) return 0;
            if (1 <= a) return 1;
            var b = 50 * a;
            a = Math.floor(b);
            b -= a;
            return d[a] * (1 - b) + d[a + 1] * b
        }
    }
    Ux(0, 0, .58);
    var Vx = Ux(.52, 0, .48);
    Ux(.52, 0, .25);
    var Wx = Ux(.36, .67, .533);
    Ux(.24, .67, .533);
    Ux(.56, 1, .56);
    Ux(.91, 1, .82);

    function Xx(a, b) {
        this.J = zs(a);
        this.L = Wx;
        this.C = Ib(N(se(a), 1));
        this.B = Ib(Ae(se(a)));
        this.I = Ib(N(se(a), 2));
        this.K = Lb(this.C, Ib(N(b, 1)));
        this.F = Lb(this.I, Ib(N(b, 2)));
        this.A = Lb(this.B, Ib(Ae(b)));
        this.M = 0 != this.A || 0 != this.F || 0 != this.K ? 650 : 0
    }
    Xx.prototype.D = h("M");
    Xx.prototype.G = function(a) {
        var b = this.J,
            c = this.L(a);
        a = Ib(this.B + Gb(0, this.A, c));
        var d = Ib(this.C + Gb(0, this.K, c)),
            c = Ib(this.I + Gb(0, this.F, c));
        te(b).data[0] = a;
        te(b).data[1] = d;
        te(b).data[2] = c;
        return b
    };
    Xx.prototype.H = ca(null);

    function Yx(a, b, c, d) {
        this.B = zs(a);
        this.M = Wx;
        this.C = pe(a);
        this.A = b;
        this.K = Ae(se(a));
        var e = ue(a),
            f;
        f = pe(a) * (e.W() / Ce(e));
        var g;
        g = b * (e.W() / Ce(e));
        c -= e.W() / 2;
        this.F = Ae(se(a)) + c / e.W() * f - c / e.W() * g;
        this.J = N(se(a), 1);
        e = Ce(ue(a));
        d -= e / 2;
        this.I = Eb(N(se(a), 1) - d / e * pe(a) + d / e * b, 0, 170)
    }
    Yx.prototype.D = function() {
        return 1E3 * Math.abs(this.A - this.C) / 72
    };
    Yx.prototype.G = function(a) {
        var b = this.M(a);
        a = Gb(this.C, this.A, b);
        var c = Gb(this.K, this.F, b),
            b = Gb(this.J, this.I, b),
            d = this.B;
        te(d).data[0] = c;
        te(d).data[1] = b;
        d.data[3] = a;
        return this.B
    };
    Yx.prototype.H = ca(null);

    function Zx(a) {
        if (Tc) a = $x(a);
        else if (Wc && Uc) switch (a) {
            case 93:
                a = 91
        }
        return a
    }

    function $x(a) {
        switch (a) {
            case 61:
                return 187;
            case 59:
                return 186;
            case 173:
                return 189;
            case 224:
                return 91;
            case 0:
                return 224;
            default:
                return a
        }
    };
    var ay = {
            Ph: 6176
        },
        by = {
            Ph: 5610
        },
        cy = {
            Ph: 5611
        };

    function dy(a, b, c, d, e) {
        var f = O(new Dd(b.data[7]), 0);
        if (!f) return "";
        f = new lr(f);
        O(e, 0) && f.A.set("hl", O(e, 0));
        O(e, 1) && f.A.set("gl", O(e, 1));
        2 == L(qh(b), 1) ? a && f.A.set("cbp", "1," + Math.floor(Ae(se(a))) + ",,0,0") : c ? f.A.set("cid", c) : d && f.A.set("fid", d);
        return f.toString()
    };

    function ey(a) {
        if (!a || !K(a, 5)) return null;
        a = new Se(a.data[5]);
        var b;
        (b = !K(a, 4)) || (b = ge(new de(a.data[4])), b = /^[\s\xa0]*$/.test(null == b ? "" : String(b)));
        return b ? null : ge(new de(a.data[4]))
    };

    function fy(a) {
        this.data = a || []
    }
    H(fy, J);

    function gy(a) {
        this.data = a || []
    }
    H(gy, J);

    function hy(a, b, c, d, e) {
        var f = a.bd,
            g = a.Fe;
        a = [];
        var k = [],
            l;
        U(Ah(f), b);
        K(b, 7) && (c = dy(null, b, c, d, e), Ed(new Dd(P(Ah(f), 7)), c), Nd(new Md(P(g, 12)), c));
        c = new Ie(b.data[4]);
        d = th(b);
        for (l = 0; l < S(c, 1); l++) a.push(iy(new Je(Nc(c, 1, l))));
        for (l = 0; l < S(c, 0); l++) k.push(jy(new Je(Nc(c, 0, l))));
        S(c, 0) || (e = new gy, e.data[0] = "Photos are copyrighted by their owners", k.push(e));
        for (l = 0; l < S(c, 2); l++) k.push(jy(new Je(Nc(c, 2, l))));
        for (l = 0; l < S(c, 3); l++) e = new Je(Nc(c, 3, l)), Ic(e, 4, !0) ? k.push(jy(e)) : a.push(iy(e));
        e = sh(b);
        var m, n = !1;
        S(e, 2) ? (m = ge(new de(Nc(e, 2, 0))), n = !0) : 2 != L(qh(b), 1) && (m = "Untitled");
        l = ky(b);
        var p = !1;
        l || (l = m, p = !0);
        m = n && p;
        n = L(ph(b), 0);
        1 != n && l && (S(c, 4) && O(new Je(Nc(c, 4, 0)), 1) ? (p = new xh, Nd(Ph(p), O(new Je(Nc(c, 4, 0)), 1)), Od(Ph(p), l), a.unshift(p)) : f.data[3] = l);
        1 == n && (l && (f.data[3] = l), (c = ey(d)) ? Nh(f, c) : Gs(f) && Nh(f, "From the web"));
        for (l = m ? 1 : 0; l < S(e, 2); l++) Nh(f, ge(new de(Nc(e, 2, l))));
        ge(new de(e.data[4])) && (f.data[20] = ge(new de(e.data[4])));
        if (b = Ls(b)) m = new gy, m.data[0] = "Image capture: " + b, k.unshift(m);
        Jc(g, 11);
        nb(k,
            function(a) {
                U(new gy(Mc(g, 11)), a)
            });
        Jc(f, 18);
        nb(a, function(a) {
            U(new xh(Mc(f, 18)), a)
        })
    }

    function ky(a) {
        var b = ge(new de(sh(a).data[5])) || ge(new de(sh(a).data[6])) || ge(new de(sh(a).data[7]));
        if (b) return b;
        for (var c = 0; c < S(a, 5); c++)
            for (var d = uh(a, c), e = 0; e < S(d, 9); e++) {
                var f = new fg(Nc(d, 9, e));
                if ((b = ge(new de(f.data[2]))) && !K(f, 1)) return b
            }
        return null
    }

    function jy(a) {
        var b = new gy,
            c = ge(Ke(a)) || 1 == L(a, 3) && "From a Google User" || "";
        K(a, 2) ? (b.data[2] = O(a, 2), b.data[5] = c) : c && (b.data[0] = c);
        K(a, 1) && (b.data[1] = O(a, 1));
        return b
    }

    function iy(a) {
        var b = new xh,
            c = ge(Ke(a)) || 1 == L(a, 3) && "From a Google User" || "";
        K(a, 1) ? (c && Od(Ph(b), c), Nd(Ph(b), O(a, 1)), K(a, 2) && (Ph(b).data[2] = O(a, 2))) : c && (b.data[0] = c);
        return b
    };

    function ly(a, b) {
        return a === b
    };

    function my(a, b, c) {
        this.C = a;
        this.F = b;
        this.D = c;
        this.A = !1;
        this.B = null
    }
    var ny = 1E5;

    function oy(a, b) {
        if (!a.A) {
            var c = a.F;
            !1 === (a.D ? c.call(a.D, b) : c(b)) && a.cancel()
        }
    }
    q = my.prototype;
    q.cancel = function() {
        this.A = !0
    };
    q.key = h("B");
    q.ra = function() {
        this.C && this.Ub();
        this.C = null
    };
    q.listen = function() {
        if (null == this.B && this.C) {
            this.A = !1;
            this.B = ny++;
            var a = this.C;
            a.C || (a.C = {});
            a.C[this.key()] = this;
            (a = a.A.B) && a.A.push(this)
        }
    };
    q.Ub = function() {
        null != this.B && this.C && (py(this.C, this.B), this.B = null)
    };

    function qy() {
        this.A = [];
        this.B = !1
    };

    function ry() {
        this.H = sy++;
        this.F = null;
        this.D = {};
        this.C = null;
        this.A = this;
        this.B = null
    }

    function ty() {
        this.B = 0;
        this.A = [];
        this.C = !1
    }
    var sy = 1E5;

    function uy(a, b) {
        if (a.C)
            for (var c in a.C) c = Number(c), oy(a.C[c], b);
        for (var d in a.D) d = Number(d), uy(a.D[d], b)
    }

    function py(a, b) {
        if (null !== b && void 0 !== b && (a.C && delete a.C[b], a = a.A.B)) {
            var c = qb(a.A, function(a) {
                return a.key() == b
            });
            c && (c.cancel(), a.C = !0)
        }
    }

    function vy(a, b) {
        b !== a.F && (a.F && delete a.F.D[a.H], a.F = b, a.A.B = null, wy(a, a), b && b.A !== a && (b.D[a.H] = a, a.F = b, b.A.B = null, wy(a, b.A)))
    }

    function wy(a, b) {
        a.A = b;
        for (var c in a.D) c = Number(c), wy(a.D[c], b)
    }

    function xy(a) {
        if (a.B) {
            if (a.B.C && 0 == a.B.B && (a = a.B, a.C && 0 == a.B)) {
                for (var b = a.A, c = 0, d = b.length; c < d; c++) b[c].A && a.A.splice(c, 1), --c, --d;
                a.C = !1
            }
        } else b = new ty, yy(a, b), a.B = b
    }

    function yy(a, b) {
        if (a.C)
            for (var c in a.C) c = Number(c), b.A.push(a.C[c]);
        for (var d in a.D) d = Number(d), yy(a.D[d], b)
    };

    function zy(a, b, c) {
        ry.call(this);
        this.I = b;
        this.G = c
    }
    H(zy, ry);

    function Ay(a, b, c) {
        return void 0 === b || void 0 === c ? void 0 === b && void 0 === c : a.I.call(void 0, b, c)
    }

    function By(a, b, c) {
        var d = !Ay(a, a.get(), b.get());
        vy(a, b);
        a.G = void 0;
        d && uy(a, c)
    }

    function Cy(a, b) {
        var c = a.F && y(a.get());
        vy(a, null);
        c && uy(a, b)
    }
    zy.prototype.get = function() {
        return this.A.G
    };
    zy.prototype.listen = function(a, b) {
        a = new my(this, a, b);
        a.listen();
        return a
    };
    zy.prototype.set = function(a, b) {
        var c = this.A;
        Ay(this, a, c.G) || (c.G = a, Dy(this, b))
    };

    function Dy(a, b) {
        xy(a.A);
        a = a.A.B;
        var c = a.A;
        a.B += 1;
        for (var d = 0, e = c.length; d < e; d++) {
            var f = c[d];
            f.A || oy(f, b)
        }--a.B
    };

    function Ey() {
        this.A = ly
    }

    function Fy() {
        return new Ey
    }

    function Gy(a, b) {
        return new zy(0, a.A, b)
    }

    function Hy(a, b) {
        return new zy(0, a.A, b)
    };
    var Iy = Fy();
    var Jy = Fy();
    var Ky = Fy();
    var Ly = Fy();
    var My = {};

    function Ny(a, b, c, d) {
        var e = w.setTimeout(function() {
            var b = My[e];
            delete My[e];
            try {
                a.call(w, b.oa)
            } catch (g) {
                throw pj(g);
            }
            b.oa.done(b.qf)
        }, b);
        c.la(d);
        b = {};
        b.oa = c;
        b.qf = d;
        My[e] = b;
        return e
    }

    function Oy(a) {
        w.clearTimeout(a);
        var b = My[a];
        b && (b.oa.done(b.qf), delete My[a])
    };

    function Py() {
        this.A = this.B = this.C = null
    }
    Py.prototype.start = function(a, b, c) {
        this.A = a;
        this.C = b;
        this.B = Ny(E(this.D, this), 200, c, "sceneContZoomStart")
    };
    Py.prototype.D = function(a) {
        this.cancel(a)
    };
    Py.prototype.cancel = function(a) {
        this.A && (this.A.cancel(a), this.C = this.B = this.A = null)
    };

    function Qy(a, b) {
        var c = !1;
        !b || ue(a).W() === b.W() && Ce(ue(a)) === Ce(b) || (c = !0, U(ve(a), b));
        b = 75;
        K(a, 3) && (b = Eb(pe(a), 1, 179));
        b != pe(a) && (a.data[3] = b, c = !0);
        K(se(a), 1) || (te(a).data[1] = 90, c = !0);
        a = re(a);
        b = xe(a);
        b = 90 < b ? 90 : -90 > b ? -90 : b;
        b != xe(a) && (a.data[2] = b, c = !0);
        b = Gx(we(a));
        b != we(a) && (a.data[1] = b, c = !0);
        return c
    };

    function Ry(a, b, c, d, e) {
        this.A = a;
        this.A.A.M = !1;
        this.wa = null;
        this.X = !1;
        this.Ua = this.Ta = this.Db = this.Wa = 0;
        this.Fb = e;
        this.Ga = d;
        this.K = Gy(Ly);
        this.S = Hy(Iy);
        this.S.listen(this.Fk, this);
        this.L = Gy(Iy);
        this.L.listen(this.Gk, this);
        this.B = Hy(Ky);
        this.H = Hy(Jy, new fy);
        this.$ = new od;
        this.$.data[0] = O(new Ee(b.data[16]), 0);
        this.$.data[1] = O(new Ee(b.data[16]), 1);
        this.N = new Py;
        this.F = this.J = this.D = null;
        this.O = [];
        this.C = this.ha = this.U = null;
        this.aa = new Float64Array(2);
        this.da = new Float64Array(2);
        this.Ma = F();
        this.Y = !1;
        this.ja = !0;
        this.Pb = !1;
        this.Zb = Ic(b, 88);
        this.ga = !0;
        this.M = null;
        this.G = this;
        this.na = this.A;
        this.P = this;
        this.I = this;
        this.xa = null
    }
    var Sy = new ne,
        Ty = new Float64Array(2),
        Uy = rl(),
        Vy = rl();
    q = Ry.prototype;
    q.Fg = function(a, b) {
        Wy(this.A, K(b, 8) ? $s(b) : null)
    };
    q.nf = function(a, b) {
        By(a, this.H, b)
    };
    q.Hc = function(a, b) {
        b = Qy(a, b || void 0);
        a = Xy(a);
        return b || a
    };

    function Xy(a) {
        var b = !1,
            c = pe(a),
            d = Eb(c, 15, 90);
        d !== c && (b = !0, a.data[3] = d);
        return b
    }
    q.Gg = function(a, b, c, d, e) {
        if (this.F) return a && b ? (d = new le, U(d, a), this.Hc(d, ue(this.U), c.B), d = Oc(b, this.ha) && Oc(d, this.U)) : d = !1, d && this.O.push(e), d;
        var f = this.S.get(),
            g = this.B.get();
        b && Wy(this.A, K(b, 8) ? $s(b) : null);
        if (b && 1 === Xs(b) && !Ms(Ys(b), Ys(g)) && 2 === c.A) return !1;
        if (!b || Xs(b) == Xs(g)) return Yy(this, a, b, c, d, e), !0;
        if (c = 2 != c.A) c = this.wa, c = !!c.Db.get() && !c.Ga.get();
        return c && b && zx(b) ? (Zy(this, d), a = a || f, b = b || g, g = this.K.get(), Jc(a, 1), $y(this, d), az(g, a, b, d, e), !0) : !1
    };

    function Yy(a, b, c, d, e, f) {
        var g = a.S.get(),
            k = a.B.get();
        Zy(a, e);
        if (c && !wx(k, c) || b) {
            var l = new le;
            b && (U(l, b), a.Hc(l, ue(g), d.B));
            b || (l = g);
            if (!c || ux(k, c)) {
                if (2 === d.A) {
                    U(g, l);
                    Dy(a.S, e);
                    f(e);
                    return
                }
                c || (c = k)
            }
            b = null;
            k && K(k, 4) && (b = Ys(k));
            k = new wh;
            U(k, Ys(c));
            b && Ms(b, k) && (Jx(a.A, k, e), bz(a, k, e));
            g ? (g = qe(g), dm(we(g), xe(g), ye(g), Uy)) : Uy[0] = Uy[1] = Uy[2] = 0;
            g = qe(l);
            dm(we(g), xe(g), ye(g), Vy);
            g = Math.sqrt(Bl(Uy, Vy));
            cz(a, l, c, 2 == d.A || 800 < g, f, e)
        } else U(a.B.get(), c), Dy(a.B, e), f(e)
    }
    q.Zk = function(a) {
        this.D = null;
        a.ua("thp1")
    };
    q.Ec = ca(!0);
    q.Gc = function(a, b, c, d, e) {
        if (this.ga) {
            var f = this.S.get(),
                g = this.K.get();
            if (0 !== a && !this.D && !this.F) {
                b.Vc("zoom");
                var k = pe(f);
                a = 0 < a ? 0 : 1;
                var l = 0 === a ? 15 : 90;
                c = y(c) ? c : ue(f).W() / 2;
                d = y(d) ? d : Ce(ue(f)) / 2;
                e ? this.N.A ? (g = this.N, g.C === a ? (Oy(g.B), g.B = Ny(E(g.D, g), 200, b, "sceneContZoomTickle")) : g.cancel(b)) : (e = l, e !== k && (f = new Yx(f, e, c, d), g = dz(g, f, b, E(this.Og, this)), this.N.start(g, a, b))) : (this.N.cancel(b), e = 0 === a ? k / 2 : 2 * k, e = 0 === a ? Math.max(l, e) : Math.min(l, e), 10 >= Math.abs(e - l) && (e = l), e !== k && (f = new Yx(f, e, c, d), this.J = dz(g,
                    f, b, E(this.Og, this))))
            }
        }
    };
    q.Ed = function(a, b) {
        if (this.ga) {
            var c = this.S.get(),
                d = pe(c),
                d = d * Math.pow(2, -a);
            c.data[3] = d;
            Xy(c) && Dy(this.S, b);
            if (a = this.B.get()) d = new le, U(d, c), To(this, "user-input-event", b, {
                type: "zoom",
                S: d,
                contentType: Xs(a)
            })
        }
    };
    q.Og = function(a) {
        this.J = null;
        var b = this.S.get(),
            c = this.B.get();
        if (b && c) {
            var d = new le;
            U(d, b);
            To(this, "user-input-event", a, {
                type: "zoom",
                S: d,
                contentType: Xs(c)
            })
        }
    };
    q.Rg = function(a, b) {
        ez(this, a);
        fz(this.A, a.x, a.y, b);
        this.D && (this.D.cancel(b), b.ua("thp1"));
        var c, d = this.A;
        c = a.x;
        a = a.y;
        if (d.N) a: {
            for (var d = d.O, e = d.F.O, f = 0; f < d.C.length; f++) {
                var g = gz;
                Fl(g, d.F.$);
                var k;
                b: {
                    k = d.C[f].shape;
                    var l = c,
                        m = a,
                        n = e,
                        p = -Infinity,
                        r = Infinity,
                        u = -Infinity,
                        t = Infinity;
                    if (hz(k)) k = !1;
                    else {
                        Ml(g, iz(k), g);
                        for (var v = 0; v < k.A.length / 3; v++) {
                            jz[0] = k.A[3 * v + 0];
                            jz[1] = k.A[3 * v + 1];
                            jz[2] = k.A[3 * v + 2];
                            jz[3] = 1;
                            Ql(g, jz, jz);
                            if (0 > jz[3]) {
                                k = !1;
                                break b
                            }
                            wl(jz, 1 / jz[3], jz);
                            Ol(n, jz, jz);
                            jz[0] < r && (r = jz[0]);
                            jz[1] < t && (t =
                                jz[1]);
                            jz[0] > p && (p = jz[0]);
                            jz[1] > u && (u = jz[1])
                        }
                        k = l <= p && l >= r && m <= u && m >= t ? !0 : !1
                    }
                }
                if (k) {
                    c = d.C[f].target;
                    break a
                }
            }
            c = null
        }
        else c = null;
        c && kz(this, c, b)
    };
    q.Ug = function(a, b) {
        ez(this, a);
        fz(this.A, a.x, a.y, b)
    };

    function lz(a, b) {
        a.A.D.Yd(Ty);
        return Eb(b, Ty[0], Ty[1])
    }
    q.Ve = function(a, b) {
        ez(this, a);
        if (this.X) {
            mz(this, a.x, a.y);
            var c = this.S.get(),
                d = ue(c).W(),
                e = 1 / Math.tan(Jb(pe(c) / 2)),
                f = Ce(ue(c)) / 2,
                d = d / 2,
                d = Kb(Math.atan2((a.x - d) / f, e) - Math.atan2((this.Wa - d) / f, e));
            a = lz(this, this.Ua + Kb(Math.atan2((a.y - f) / f, e) - Math.atan2((this.Db - f) / f, e)));
            e = Ib(this.Ta - d);
            te(c).data[1] = a;
            te(c).data[0] = e;
            Dy(this.S, b)
        } else fz(this.A, a.x, a.y, b)
    };
    q.Ue = function(a, b) {
        ez(this, a);
        b.Vc("pan");
        this.X = !0;
        this.A.F && nz(this.Fb, new tx(this.A, !1), 100);
        this.Wa = a.x;
        this.Db = a.y;
        this.Ta = Ae(se(this.S.get()));
        this.Ua = N(se(this.S.get()), 1);
        b = this.da;
        var c = a.y;
        b[0] = a.x;
        b[1] = c;
        a = this.aa;
        a[0] = 0;
        a[1] = 0;
        this.Ma = F()
    };
    q.Sg = function(a, b) {
        ez(this, a);
        if (!this.F) {
            var c = this.S.get();
            mz(this, a.x, a.y);
            if (this.X) {
                var d = -1 * this.aa[0],
                    e = this.aa[1];
                if (.25 < Math.sqrt(d * d + e * e)) {
                    a = this.K.get();
                    var f = se(c),
                        e = N(f, 1) + 10 * e,
                        e = lz(this, e);
                    Sy.data[1] = e;
                    Sy.data[0] = Ae(f) + 10 * d;
                    d = new Xx(c, Sy);
                    b.ua("thp0");
                    this.D = dz(a, d, b, E(this.Zk, this))
                }
            }
            this.X = !1;
            this.A.F && nz(this.Fb, new tx(this.A, !0), 100);
            if (a = this.B.get()) d = new le, U(d, c), To(this, "user-input-event", b, {
                type: "rotate",
                S: d,
                contentType: Xs(a)
            })
        }
    };

    function ez(a, b) {
        b = !(b && ("touchstart" === b.type || "touchmove" === b.type || "touchend" === b.type || "touchcancel" === b.type)) && a.ja && !oz(a.A, b.x, b.y);
        a.A.A.M = b
    }

    function mz(a, b, c) {
        var d = F(),
            e = d - a.Ma;
        if (0 < e) {
            var f = a.aa,
                g = c - a.da[1],
                k = Math.exp(-e / 32);
            f[0] = k * f[0] + (1 - k) * (b - a.da[0]) / e;
            f[1] = k * f[1] + (1 - k) * g / e
        }
        e = a.da;
        e[0] = b;
        e[1] = c;
        a.Ma = d
    }
    q.Te = function(a, b) {
        return this.F && this.F.A || this.J && this.J.A || this.D && this.D.A || this.C && this.C.A ? !1 : oz(this.A, a, b) ? !0 : this.ja ? !!this.A.A.B : !1
    };
    q.Lb = ba("ja");
    q.Tg = function(a, b, c) {
        return c
    };
    q.Se = function(a, b) {
        ez(this, a);
        if (this.Te(a.x, a.y, b) && this.A.A && (fz(this.A, a.x, a.y, b), !oz(this.A, a.x, a.y))) {
            var c = new wh;
            U(c, this.A.A.B);
            var d = pz(this.A.A);
            if (c) {
                var e = this;
                qz(this);
                this.M = rz(this.A, c, function(a, b, k, l) {
                    a && (U(c, l), b && U(re(Bh(c)), qe(b)), sz(e, c, d, k), tz(e, c))
                }, b)
            }
        }
    };

    function tz(a, b, c, d) {
        var e = O(b, 9);
        b = O(b, 10);
        e && b && a.Ga.F(e, b, ay, void 0, !0, c, d)
    }

    function kz(a, b, c) {
        c.ob("scene", "move_camera");
        qz(a);
        a.M = rz(a.A, b, function(c, e, f, g) {
            c && (U(b, g), Jc(Bh(b), 1), e && U(re(Bh(b)), qe(e)), sz(a, b, null, f), tz(a, b, 4))
        }, c)
    }

    function qz(a) {
        a.M && (a.M.A(), a.M = null)
    }

    function uz(a, b, c, d) {
        a.C && a.C.cancel(c);
        a.D && a.D.cancel(c);
        var e = a.S.get(),
            f = Ae(se(e)),
            g = vz(a.A, (b ? f : f + 180) % 360);
        b = a.B.get();
        var k, l;
        e && b && (k = new le, l = Xs(b));
        g && (c.ob("scene", "move_camera"), qz(a), a.M = rz(a.A, g, function(b, c, e, f) {
            b && (U(g, f), c && U(re(Bh(g)), qe(c)), U(te(Bh(g)), se(a.S.get())), sz(a, g, null, e), tz(a, g, 24, d), y(k) && y(l) && To(a, "user-input-event", e, {
                type: "pan",
                S: k,
                contentType: l
            }))
        }, c))
    }

    function wz(a, b, c, d) {
        if (!a.C && !a.F) {
            d.Vc("pan");
            a.D && a.D.cancel(d);
            var e = a.S.get(),
                f = a.K.get();
            c && d.ua("pan0");
            c = new Px(e, b, c);
            a.C = dz(f, c, d, E(a.Pk, a, b))
        }
    }
    q.Pk = function(a, b) {
        this.C.G ? (this.C = null, b.ua("pan1")) : (this.C = null, wz(this, a, !1, b));
        a = this.S.get();
        var c = this.B.get();
        if (a && c) {
            var d = new le;
            U(d, a);
            To(this, "user-input-event", b, {
                type: "rotate",
                S: d,
                contentType: Xs(c)
            })
        }
    };
    q.Qg = function(a, b) {
        ez(this, a);
        this.Se(a, b)
    };

    function sz(a, b, c, d) {
        if (d.Dg("scene", "move_camera")) {
            var e = a.S.get(),
                f = Bh(b);
            c ? dp(f, c) : K(f, 1) || (c = te(f), e ? U(c, se(e)) : c.data[1] = 90);
            e = a.B.get();
            b = Bx(b);
            U(new Ts(P(b, 8)), $s(e));
            cz(a, f, b, !1, C, d)
        }
    }

    function cz(a, b, c, d, e, f) {
        if (!(a.J || a.D || a.F || a.C) && a.Y) {
            var g = a.K.get();
            if (g) {
                var k = a.S.get();
                if (k) {
                    var l = a.B.get();
                    if (l) {
                        var m = Ys(l),
                            n = Ys(c),
                            p = zs(b);
                        a.Pb ? K(p, 3) || (p.data[3] = pe(k)) : p.data[3] = 75;
                        var m = xz(a.A, m, f),
                            r = xz(a.A, n, f);
                        null != m && null != r && (m = ye(qe(b)) + (r - m), ze(re(p), m));
                        d = new Hx(k, l, p, c, d, a.A, f);
                        a.U = b;
                        a.ha = c;
                        f.ua("c2g0");
                        a.O.push(e);
                        a.F = dz(g, d, f, function(b) {
                            b.ua("c2g1");
                            a.F = null;
                            a.U = null;
                            a.ha = null;
                            bz(a, n, b);
                            for (var c = 0; c < a.O.length; c++) a.O[c](b);
                            a.O = []
                        })
                    }
                }
            }
        }
    }
    q.wb = function(a, b) {
        this.Y = !0;
        this.wa = a;
        yz(a, this.A, b);
        this.A.wb(b);
        var c = this.B.get();
        a = this.S.get();
        Jc(Zs(c), 17);
        c = Zs(c);
        U(Bh(c), a);
        Jx(this.A, c, b);
        var d = zz(this.A);
        d ? (U(Bh(d), a), bz(this, d, b)) : bz(this, c, b)
    };
    q.Wf = function(a) {
        this.Y = !1;
        Zy(this, a);
        qz(this);
        $y(this, a);
        var b = this.B.get();
        b && (Zs(b).data[3] = "", Jc(Zs(b), 17));
        Az(this.A, a);
        this.wa = null
    };

    function Zy(a, b) {
        a.D && (a.D.cancel(b), a.D = null);
        a.J && (a.J.cancel(b), a.J = null);
        a.F && (a.F.cancel(b), a.F = null);
        a.C && (a.C.cancel(b), a.C = null);
        a.N.A && a.N.cancel(b)
    }
    q.Fk = function(a) {
        var b = this.S.get();
        b && Bz(this.A, b, a)
    };
    q.Gk = function(a) {
        if (this.S.get()) {
            if (zz(this.A)) {
                this.L.get() && (this.H.get(), Cz(this), Dy(this.H, a));
                var b;
                b = this.A;
                b = b.C ? b.C.fa() : null;
                var c = this.S.get();
                c && K(c, 0) && K(qe(c), 0) && b && K(b, 0) && K(qe(b), 0) && .1 < Math.abs(ye(qe(c)) - ye(qe(b))) && (c = new Ex, c.A = 2, Yy(this, b, null, c, a, C))
            }
            a = this.A;
            a.aa = !1;
            fp(a.G)
        }
    };

    function bz(a, b, c) {
        qz(a);
        a.M = rz(a.A, b, function(c, e, f, g) {
            if (a.Y)
                if (c) {
                    c = new wh;
                    U(c, g);
                    e && (U(re(Bh(c)), qe(e)), U(te(Bh(c)), se(e)));
                    Dz(a, c, f);
                    g = g.ia();
                    var d = a.$;
                    e = {
                        Fe: new fy,
                        bd: new wh
                    };
                    U(e.bd, c);
                    if (g && (K(g, 6) || K(g, 4) || K(g, 3) || K(g, 9) || K(g, 1) || K(g, 2) || K(g, 8) || K(g, 0) || K(g, 7))) {
                        var l;
                        if (c && K(c, 11)) {
                            l = new lr(O(new Md(c.data[11]), 0));
                            var m = l.A.get("cid");
                            m || (m = (new lr(qr(l.A.toString()))).A.get("cid"));
                            l = m ? m : null
                        } else l = null;
                        m = null;
                        0 < S(c, 15) && 0 == L(new Af(Oh(c).data[0]), 8) && (m = O(new Af(Oh(c).data[0]), 0));
                        Jc(e.bd,
                            3);
                        Jc(e.bd, 17);
                        hy(e, g, l, m, d)
                    }
                    g = a.B.get();
                    d = a.H.get();
                    c = Zs(g);
                    U(c, e.bd);
                    e.Fe && U(d, e.Fe);
                    Ax(Bx(c), g);
                    Cz(a);
                    if (a.Zb) {
                        e = a.A;
                        if (g = e.C) Ms(Ys(g.Da()), c), g.Ib(c), Ez(e, f);
                        Dy(a.S, f)
                    }
                    e = new qy;
                    c = a.B;
                    xy(c.A);
                    e.B || e.A.push.apply(e.A, c.A.B.A);
                    c = a.H;
                    xy(c.A);
                    e.B || e.A.push.apply(e.A, c.A.B.A);
                    e.B = !0;
                    c = e.A;
                    g = 0;
                    for (d = c.length; g < d; g++) l = c[g], l.A || oy(l, f);
                    e.A = [];
                    e.B = !1
                } else e = new wh, U(e, b), Jc(e, 0), K(e, 21) && Jc(Ah(e), 1), Dz(a, e, f)
        }, c)
    }

    function Cz(a) {
        var b = a.L.get(),
            c = a.B.get(),
            d = a.H.get();
        b && d && c && (a = dy(b, Ys(c).ia(), null, null, a.$), Nd(new Md(P(d, 12)), a))
    }

    function $y(a, b) {
        var c = a.H.get();
        c.data[0] = "";
        c.data[2] = "";
        Dy(a.H, b)
    }

    function Dz(a, b, c) {
        var d = a.S.get();
        if (d) {
            var e = a.B.get();
            Ax(Bx(b), e) && Dy(a.B, c);
            var e = !1,
                f = qe(b.fa());
            if (xe(qe(d)) !== xe(f) || we(qe(d)) !== we(f) || ye(qe(d)) !== ye(f)) U(re(d), f), e = !0;
            f = b.ia();
            0 < S(f, 5) && K(uh(f, 0), 10) && (f = new Bf(uh(f, 0).data[10]), Js(f, d) && (e = !0));
            e && Dy(a.S, c);
            K(b, 0) && a.L.get() && (a.H.get(), Cz(a), Dy(a.H, c))
        }
    }
    q.Hg = function(a, b) {
        var c = this.S.get();
        U(ve(c), a);
        Dy(this.S, b);
        if (a = this.B.get()) {
            var d = new le;
            U(d, c);
            To(this, "user-input-event", b, {
                type: "resize",
                S: d,
                contentType: Xs(a)
            })
        }
    };
    q.Ge = ca("pa");

    function Fz(a, b, c, d, e, f, g) {
        c.load("pa", b, function(b) {
            a(new Ry(b, d, 0, f, g))
        })
    };

    function Gz(a, b) {
        V.call(this, "PNI", wb(arguments))
    }
    H(Gz, V);

    function Hz(a, b, c, d) {
        b = new Wu(c, d);
        a(b)
    };

    function Iz(a, b, c, d, e, f) {
        V.call(this, "SCPR", wb(arguments))
    }
    H(Iz, V);

    function Jz() {
        this.C = [0, 0];
        this.A = [0, 0];
        this.G = 0;
        this.F = this.B = null;
        this.D = {}
    }

    function Kz(a, b, c, d, e, f, g) {
        this.handle = a;
        this.item = b;
        this.D = c;
        this.F = d;
        this.A = null;
        this.next = e;
        this.B = f;
        this.C = g
    }
    q = Jz.prototype;
    q.add = function(a, b, c, d, e) {
        c = c || 0;
        d = d || 0;
        if (c > this.C[0] || d > this.C[1]) return -1;
        var f = this.G++;
        a = new Kz(f, a, b, e || null, this.B, c, d);
        this.D[f] = a;
        this.B && (this.B.A = a);
        this.B = a;
        this.A[0] += c;
        this.A[1] += d;
        null == this.F && (this.F = a);
        Lz(this);
        return f
    };
    q.get = function(a) {
        return (a = this.D[a]) ? a.item : void 0
    };

    function Lz(a) {
        for (var b = a.F; b && (a.A[0] > a.C[0] || a.A[1] > a.C[1]);) {
            var c = b,
                b = b.A;
            if (a.A[0] > a.C[0] && 0 < c.B || a.A[1] > a.C[1] && 0 < c.C || 0 == c.B && 0 == c.C) c.D && c.D.call(c.F, c.handle, c.item, !1), a.remove(c.handle)
        }
    }
    q.remove = function(a) {
        var b = this.D[a];
        b && (b.A ? b.A.next = b.next : this.B = b.next, b.next ? b.next.A = b.A : this.F = b.A, b.A = b.next = b.item = null, b.handle = -1, delete this.D[a], this.A[0] -= b.B, this.A[1] -= b.C)
    };
    q.contains = function(a) {
        return a in this.D
    };
    q.clear = function() {
        for (; this.B;) {
            var a = this.B;
            a.D.call(a.F, a.handle, a.item, !0);
            this.remove(a.handle)
        }
        Object.keys(this.D)
    };

    function Mz() {
        il.call(this);
        this.A = new Float32Array(0);
        this.D = [1, 1, 1, 1];
        this.C = null;
        this.B = new Nz;
        this.G = -1;
        this.F = null
    }
    H(Mz, il);
    var jz = Cl();

    function Nz() {
        this.hidden = !1;
        this.A = El();
        Ll(this.A)
    }

    function Oz() {
        for (var a = new Mz, b = new Float32Array(150), c = 0; 50 > c; c++) b[3 * c + 0] = Math.sin(c / 50 * Math.PI * 2), b[3 * c + 1] = Math.cos(c / 50 * Math.PI * 2), b[3 * c + 2] = 0;
        a.A = b;
        return a
    }

    function Pz() {
        var a = new Mz;
        a.A = new Float32Array([-.5, .5, 0, .5, .5, 0, .5, -.5, 0, -.5, -.5, 0]);
        return a
    }
    Mz.prototype.Aa = function(a) {
        return null.Aa(a)
    };

    function Qz(a, b) {
        for (var c = 0; c < b.length; c++) {
            var d = b[c];
            Ml(iz(d), iz(a), iz(d));
            Rz(d);
            d.B = a.B
        }
    }

    function Sz(a, b) {
        a.D[0] = b[0];
        a.D[1] = b[1];
        a.D[2] = b[2];
        a.D[3] = b[3]
    }

    function hz(a) {
        return a.B.hidden || 0 == a.D[3] && (!a.C || 0 == a.C[3])
    }

    function iz(a) {
        return a.B.A
    }

    function Rz(a) {
        for (var b = rl(), c = iz(a), d = 0; d < a.A.length / 3; d++) b[0] = a.A[3 * d + 0], b[1] = a.A[3 * d + 1], b[2] = a.A[3 * d + 2], Ol(c, b, b), a.A[3 * d + 0] = b[0], a.A[3 * d + 1] = b[1], a.A[3 * d + 2] = b[2];
        Ll(iz(a))
    }

    function Tz(a, b, c, d) {
        Rl(iz(a), b, c, d);
        Nl(iz(a), iz(a))
    }
    Mz.prototype.scale = function(a, b, c) {
        Ul(iz(this), a, b, c)
    };

    function Uz(a, b) {
        Ul(iz(a), b, b, b)
    }
    Mz.prototype.translate = function(a, b, c) {
        Tl(iz(this), a, b, c)
    };
    Mz.prototype.rotate = function(a, b, c, d) {
        var e = iz(this),
            f = e[0],
            g = e[1],
            k = e[2],
            l = e[3],
            m = e[4],
            n = e[5],
            p = e[6],
            r = e[7],
            u = e[8],
            t = e[9],
            v = e[10],
            x = e[11],
            D = Math.cos(a),
            z = Math.sin(a),
            A = 1 - D;
        a = b * b * A + D;
        var B = b * c * A + d * z,
            Q = b * d * A - c * z,
            M = b * c * A - d * z,
            G = c * c * A + D,
            T = c * d * A + b * z,
            R = b * d * A + c * z;
        b = c * d * A - b * z;
        d = d * d * A + D;
        e[0] = f * a + m * B + u * Q;
        e[1] = g * a + n * B + t * Q;
        e[2] = k * a + p * B + v * Q;
        e[3] = l * a + r * B + x * Q;
        e[4] = f * M + m * G + u * T;
        e[5] = g * M + n * G + t * T;
        e[6] = k * M + p * G + v * T;
        e[7] = l * M + r * G + x * T;
        e[8] = f * R + m * b + u * d;
        e[9] = g * R + n * b + t * d;
        e[10] = k * R + p * b + v * d;
        e[11] = l * R + r * b + x * d
    };

    function Vz(a, b) {
        var c = a.A.length;
        a.F = b.createBuffer();
        a.G = Wz(b.B, function() {
            var c = a.F;
            c && b.deleteBuffer(c);
            a.F = null
        }, c);
        b.bindBuffer(34962, a.F);
        b.bufferData(34962, a.A, 35044)
    }

    function Xz(a, b) {
        Yz(b.B, a.G);
        return a.F
    };

    function Zz(a) {
        var b = a[0],
            c = a[1],
            d = a[2];
        a = a[3];
        return 0 >= a | (b > +a) << 1 | (b < -a) << 2 | (c > +a) << 3 | (c < -a) << 4 | (d > +a) << 5 | (d < -a) << 6
    };

    function $z(a, b) {
        this.name = a;
        this.A = b
    }

    function aA(a) {
        return bA(a.A).getAttribLocation(a.name)
    }
    $z.prototype.vertexAttribPointer = function(a, b, c, d, e, f) {
        var g = this.A.getContext(),
            k = aA(this);
        g.vertexAttribPointer(k, a, b, c, d, e);
        f && g.enableVertexAttribArray(k)
    };

    function cA() {
        this.M = !1;
        this.J = this.C = null
    }
    cA.prototype.wa = h("M");
    cA.prototype.ra = function() {
        if (!this.M) {
            this.M = !0;
            this.ea();
            if (this.C) {
                for (var a = 0; a < this.C.length; ++a) this.C[a].ra();
                this.C = null
            }
            if (this.J) {
                for (a = 0; a < this.J.length; ++a) this.J[a]();
                this.J = null
            }
        }
    };
    cA.prototype.ea = aa();

    function dA(a, b, c) {
        cA.call(this);
        this.A = a;
        this.D = null;
        this.I = b;
        this.G = c;
        a = this.F = new wq;
        this.C || (this.C = []);
        this.C.push(a);
        yq(this.F, this.A.C, "webglcontextrestored", this.H, !1, this)
    }
    H(dA, cA);
    dA.prototype.H = function() {
        this.D = null
    };

    function eA(a, b, c) {
        c = a.createShader(c);
        a.shaderSource(c, b);
        a.compileShader(c);
        return c
    }
    dA.prototype.getContext = h("A");

    function bA(a) {
        if (!a.D) {
            var b = eA(a.A, a.I, 35633),
                c = eA(a.A, a.G, 35632);
            a.D = a.A.createProgram();
            a.D.attachShader(b);
            a.D.attachShader(c);
            a.D.be()
        }
        return a.D
    }

    function fA(a) {
        a.A.useProgram(bA(a))
    };

    function gA(a, b) {
        this.name = a;
        this.A = b
    }

    function hA(a) {
        return bA(a.A).getUniformLocation(a.name)
    }

    function iA(a, b) {
        gA.call(this, a, b)
    }
    H(iA, gA);
    iA.prototype.set = function(a) {
        this.A.getContext().uniform1i(hA(this), a)
    };

    function jA(a, b) {
        gA.call(this, a, b)
    }
    H(jA, gA);
    jA.prototype.set = function(a) {
        this.A.getContext().uniform1f(hA(this), a)
    };

    function kA(a, b) {
        gA.call(this, a, b)
    }
    H(kA, gA);
    kA.prototype.set = function(a, b) {
        this.A.getContext().uniform2f(hA(this), a, b)
    };

    function lA(a, b) {
        gA.call(this, a, b)
    }
    H(lA, gA);
    lA.prototype.set = function(a, b, c) {
        this.A.getContext().uniform3f(hA(this), a, b, c)
    };

    function mA(a, b) {
        gA.call(this, a, b)
    }
    H(mA, gA);
    mA.prototype.set = function(a, b, c, d) {
        this.A.getContext().uniform4f(hA(this), a, b, c, d)
    };

    function nA(a, b) {
        a.A.getContext().uniform4fv(hA(a), b)
    }

    function oA(a, b) {
        gA.call(this, a, b)
    }
    H(oA, gA);

    function pA(a, b) {
        a.A.getContext().uniformMatrix4fv(hA(a), !1, b)
    };

    function qA(a) {
        dA.call(this, a, "varying vec2 a;varying float b;uniform mat4 matrixClipFromModel;uniform vec2 modelToPixelScale,iconSize;attribute vec3 vert;uniform vec3 pivot;uniform float opacity;void main(){a=.5*vert.xy+.5;gl_Position=matrixClipFromModel*vec4(pivot,1);if(gl_Position.z<-gl_Position.w)b=0.;else b=opacity;gl_Position=vec4(gl_Position.x/gl_Position.w+vert.x*modelToPixelScale.x,gl_Position.y/gl_Position.w+vert.y*modelToPixelScale.y,0,1);}", "precision highp float;varying vec2 a;varying float b;uniform sampler2D iconSampler;void main(){if(b==0.)discard;gl_FragColor=texture2D(iconSampler,a);gl_FragColor.w*=b;}");
        this.B = new rA(this);
        this.attributes = new sA(this)
    }
    H(qA, dA);

    function rA(a) {
        this.B = new oA("matrixClipFromModel", a);
        this.C = new kA("modelToPixelScale", a);
        this.D = new lA("pivot", a);
        this.opacity = new jA("opacity", a);
        this.A = new iA("iconSampler", a)
    }

    function sA(a) {
        this.A = new $z("vert", a)
    };

    function tA(a, b) {
        b = ((b || new uA).A ? "#define  1\n" : "") + "";
        dA.call(this, a, b + "varying vec4 a;\n#ifdef ENABLE_TEXTURE\nvarying vec2 b;\n#endif\nuniform mat4 matrixClipFromModel;uniform vec4 color;attribute vec3 vert;\n#ifdef ENABLE_TEXTURE\nattribute vec2 aTexCoord;\n#endif\nvoid main(){a=color;\n#ifdef ENABLE_TEXTURE\nb=aTexCoord;\n#endif\ngl_Position=matrixClipFromModel*vec4(vert,1);}", b + "precision highp float;varying vec4 a;\n#ifdef ENABLE_TEXTURE\nvarying vec2 b;\n#endif\n#ifdef ENABLE_TEXTURE\nuniform sampler2D texture;uniform float textureBlendFactor;\n#endif\nvoid main(){gl_FragColor=a;\n#ifdef ENABLE_TEXTURE\ngl_FragColor.rgb=mix(gl_FragColor.rgb,texture2D(texture,b).rgb,textureBlendFactor);\n#endif\n}");
        this.B = new vA(this);
        this.attributes = new wA(this)
    }
    H(tA, dA);

    function vA(a) {
        this.A = new oA("matrixClipFromModel", a);
        this.color = new mA("color", a);
        this.B = new jA("textureBlendFactor", a)
    }

    function wA(a) {
        this.A = new $z("vert", a);
        this.B = new $z("aTexCoord", a)
    }

    function uA() {
        this.A = xA[0]
    }
    var xA = [0, 1];

    function yA() {}
    ra(yA);
    yA.prototype.A = 0;

    function zA(a) {
        il.call(this);
        this.G = a || Hj();
        this.ta = null;
        this.Ka = !1;
        this.D = null;
        this.J = void 0;
        this.L = this.N = this.I = null
    }
    H(zA, il);
    q = zA.prototype;
    q.ak = yA.hg();
    q.za = function() {
        return this.ta || (this.ta = ":" + (this.ak.A++).toString(36))
    };
    q.Z = h("D");
    q.bf = function(a) {
        if (this.I && this.I != a) throw Error("Method not supported");
        zA.V.bf.call(this, a)
    };
    q.vc = function() {
        this.D = this.G.A.createElement("DIV")
    };
    q.Mb = function() {
        this.Ka = !0;
        AA(this, function(a) {
            !a.Ka && a.Z() && a.Mb()
        })
    };
    q.Mc = function() {
        AA(this, function(a) {
            a.Ka && a.Mc()
        });
        this.J && Aq(this.J);
        this.Ka = !1
    };
    q.ea = function() {
        this.Ka && this.Mc();
        this.J && (this.J.ra(), delete this.J);
        AA(this, function(a) {
            a.ra()
        });
        this.D && Yj(this.D);
        this.I = this.D = this.L = this.N = null;
        zA.V.ea.call(this)
    };

    function AA(a, b) {
        a.N && nb(a.N, b, void 0)
    }
    q.removeChild = function(a, b) {
        if (a) {
            var c = wa(a) ? a : a.za();
            this.L && c ? (a = this.L, a = (null !== a && c in a ? a[c] : void 0) || null) : a = null;
            if (c && a) {
                var d = this.L;
                c in d && delete d[c];
                tb(this.N, a);
                b && (a.Mc(), a.D && Yj(a.D));
                b = a;
                if (null == b) throw Error("Unable to set parent component");
                b.I = null;
                zA.V.bf.call(b, null)
            }
        }
        if (!a) throw Error("Child is not in parent component");
        return a
    };

    function BA() {
        this.size = 80;
        this.A = "Arial"
    };

    function CA() {
        this.Ra = [];
        this.ca = [];
        this.xb = []
    }
    CA.prototype.qc = null;
    CA.prototype.qb = null;
    CA.prototype.Bd = !0;
    var DA = [2, 2, 6, 6, 0];
    q = CA.prototype;
    q.clear = function() {
        this.Ra.length = 0;
        this.ca.length = 0;
        this.xb.length = 0;
        delete this.qc;
        delete this.qb;
        delete this.Bd;
        return this
    };
    q.vg = function(a, b) {
        0 == lb(this.Ra) ? this.xb.length -= 2 : (this.Ra.push(0), this.ca.push(1));
        this.xb.push(a, b);
        this.qb = this.qc = [a, b];
        return this
    };
    q.ug = function(a) {
        var b = lb(this.Ra);
        if (null == b) throw Error("Path cannot start with lineTo");
        1 != b && (this.Ra.push(1), this.ca.push(0));
        for (b = 0; b < arguments.length; b += 2) {
            var c = arguments[b],
                d = arguments[b + 1];
            this.xb.push(c, d)
        }
        this.ca[this.ca.length - 1] += b / 2;
        this.qb = [c, d];
        return this
    };
    q.Vf = function(a) {
        var b = lb(this.Ra);
        if (null == b) throw Error("Path cannot start with curve");
        2 != b && (this.Ra.push(2), this.ca.push(0));
        for (b = 0; b < arguments.length; b += 6) {
            var c = arguments[b + 4],
                d = arguments[b + 5];
            this.xb.push(arguments[b], arguments[b + 1], arguments[b + 2], arguments[b + 3], c, d)
        }
        this.ca[this.ca.length - 1] += b / 6;
        this.qb = [c, d];
        return this
    };
    q.close = function() {
        var a = lb(this.Ra);
        if (null == a) throw Error("Path cannot start with close");
        4 != a && (this.Ra.push(4), this.ca.push(1), this.qb = this.qc);
        return this
    };
    q.ti = function(a, b, c, d) {
        var e = this.qb[0] - a * Math.cos(Jb(c)),
            f = this.qb[1] - b * Math.sin(Jb(c)),
            g = Jb(d);
        d = Math.ceil(Math.abs(g) / Math.PI * 2);
        g /= d;
        c = Jb(c);
        for (var k = 0; k < d; k++) {
            var l = Math.cos(c),
                m = Math.sin(c),
                n = 4 / 3 * Math.sin(g / 2) / (1 + Math.cos(g / 2)),
                p = e + (l - n * m) * a,
                r = f + (m + n * l) * b;
            c += g;
            l = Math.cos(c);
            m = Math.sin(c);
            this.Vf(p, r, e + (l + n * m) * a, f + (m - n * l) * b, e + l * a, f + m * b)
        }
        return this
    };

    function EA(a, b) {
        for (var c = a.xb, d = 0, e = 0, f = a.Ra.length; e < f; e++) {
            var g = a.Ra[e],
                k = DA[g] * a.ca[e];
            b(g, c.slice(d, d + k));
            d += k
        }
    }

    function FA(a) {
        var b = new a.constructor;
        b.Ra = a.Ra.concat();
        b.ca = a.ca.concat();
        b.xb = a.xb.concat();
        b.qc = a.qc && a.qc.concat();
        b.qb = a.qb && a.qb.concat();
        b.Bd = a.Bd;
        return b
    }
    var GA = {};
    GA[0] = CA.prototype.vg;
    GA[1] = CA.prototype.ug;
    GA[4] = CA.prototype.close;
    GA[2] = CA.prototype.Vf;
    GA[3] = CA.prototype.ti;

    function HA(a) {
        if (a.Bd) return FA(a);
        var b = new CA;
        EA(a, function(a, d) {
            GA[a].apply(b, d)
        });
        return b
    }
    CA.prototype.Oa = function() {
        return 0 == this.Ra.length
    };

    function IA(a, b, c, d, e) {
        zA.call(this, e);
        this.width = a;
        this.height = b;
        this.F = c || null;
        this.H = d || null
    }
    H(IA, zA);
    IA.prototype.B = null;
    IA.prototype.gc = function(a, b) {
        this.F = a;
        this.H = b
    };

    function JA(a) {
        return a.F ? new Ej(a.F, a.H) : a.Va()
    }
    IA.prototype.Va = function() {
        return this.Ka ? Yn(this.Z()) : xa(this.width) && xa(this.height) ? new Ej(this.width, this.height) : null
    };

    function KA(a) {
        var b = a.Va();
        return b ? b.width / JA(a).width : 0
    }

    function LA(a, b, c, d, e, f) {
        c += d.size / 2;
        return a.Ee(b, 0, c, 1, c, "left", d, e, f, void 0)
    };

    function MA(a, b, c, d, e, f) {
        if (6 == arguments.length) NA(this, a, b, c, d, e, f);
        else {
            if (0 != arguments.length) throw Error("Insufficient matrix parameters");
            this.B = this.F = 1;
            this.A = this.C = this.D = this.G = 0
        }
    }

    function OA(a) {
        return new MA(a.B, a.A, a.C, a.F, a.D, a.G)
    }

    function NA(a, b, c, d, e, f, g) {
        if (!(xa(b) && xa(c) && xa(d) && xa(e) && xa(f) && xa(g))) throw Error("Invalid transform parameters");
        a.B = b;
        a.A = c;
        a.C = d;
        a.F = e;
        a.D = f;
        a.G = g;
        return a
    }
    MA.prototype.scale = function(a, b) {
        this.B *= a;
        this.A *= a;
        this.C *= b;
        this.F *= b;
        return this
    };
    MA.prototype.translate = function(a, b) {
        this.D += a * this.B + b * this.C;
        this.G += a * this.A + b * this.F;
        return this
    };
    MA.prototype.rotate = function(a, b, c) {
        var d = new MA,
            e = Math.cos(a);
        a = Math.sin(a);
        b = NA(d, e, a, -a, e, b - b * e + c * a, c - b * a - c * e);
        c = this.B;
        d = this.C;
        this.B = b.B * c + b.A * d;
        this.C = b.C * c + b.F * d;
        this.D += b.D * c + b.G * d;
        c = this.A;
        d = this.F;
        this.A = b.B * c + b.A * d;
        this.F = b.C * c + b.F * d;
        this.G += b.D * c + b.G * d;
        return this
    };
    MA.prototype.toString = function() {
        return "matrix(" + [this.B, this.A, this.C, this.F, this.D, this.G].join() + ")"
    };

    function PA(a, b) {
        il.call(this);
        this.Ob = a;
        this.fb = b;
        this[yk] = !1
    }
    H(PA, il);
    q = PA.prototype;
    q.fb = null;
    q.Ob = null;
    q.Ud = null;
    q.Z = h("Ob");
    q.addEventListener = function(a, b, c, d) {
        Vk(this.Ob, a, b, c, d)
    };
    q.removeEventListener = function(a, b, c, d) {
        cl(this.Ob, a, b, c, d)
    };
    q.ea = function() {
        PA.V.ea.call(this);
        el(this.Ob)
    };

    function QA(a, b, c, d) {
        PA.call(this, a, b);
        this.th(c);
        this.sh(d)
    }
    H(QA, PA);
    q = QA.prototype;
    q.fill = null;
    q.ff = null;
    q.sh = function(a) {
        this.fill = a;
        this.fb.Ze(this, a)
    };
    q.Vi = h("fill");
    q.th = function(a) {
        this.ff = a;
        this.fb.af(this, a)
    };
    q.Lj = h("ff");

    function RA(a, b) {
        PA.call(this, a, b)
    }
    H(RA, PA);

    function SA(a, b) {
        PA.call(this, a, b)
    }
    H(SA, PA);

    function TA(a, b, c, d) {
        QA.call(this, a, b, c, d)
    }
    H(TA, QA);

    function UA(a, b, c, d) {
        QA.call(this, a, b, c, d)
    }
    H(UA, QA);

    function VA(a) {
        PA.call(this, null, a);
        this.A = []
    }
    H(VA, RA);
    VA.prototype.clear = function() {
        this.A.length && (this.A.length = 0, WA(this.fb))
    };
    VA.prototype.appendChild = function(a) {
        this.A.push(a)
    };
    VA.prototype.B = function() {
        for (var a = 0, b = this.A.length; a < b; a++) XA(this.fb, this.A[a])
    };

    function YA(a, b, c, d, e) {
        QA.call(this, a, b, d, e);
        this.A(c)
    }
    H(YA, TA);
    YA.prototype.C = !1;
    YA.prototype.A = function(a) {
        this.D = a.Bd ? a : HA(a);
        this.C && WA(this.fb)
    };
    YA.prototype.B = function(a) {
        this.C = !0;
        a.beginPath();
        EA(this.D, function(b, c) {
            switch (b) {
                case 0:
                    a.moveTo(c[0], c[1]);
                    break;
                case 1:
                    for (b = 0; b < c.length; b += 2) a.lineTo(c[b], c[b + 1]);
                    break;
                case 2:
                    for (b = 0; b < c.length; b += 6) a.bezierCurveTo(c[b], c[b + 1], c[b + 2], c[b + 3], c[b + 4], c[b + 5]);
                    break;
                case 3:
                    throw Error("Canvas paths cannot contain arcs");
                case 4:
                    a.closePath()
            }
        })
    };

    function ZA(a, b, c, d, e, f, g, k, l, m) {
        var n = Nj("DIV", {
            style: "display:table;position:absolute;padding:0;margin:0;border:0"
        });
        QA.call(this, n, a, l, m);
        this.C = b;
        this.D = c;
        this.I = d;
        this.F = e;
        this.J = f;
        this.G = g || "left";
        this.H = k;
        this.A = Nj("DIV", {
            style: "display:table-cell;padding: 0;margin: 0;border: 0"
        });
        c = this.D;
        k = this.F;
        d = this.I;
        e = this.J;
        l = this.G;
        f = this.H;
        b = this.Z().style;
        g = KA(this.fb);
        m = this.fb;
        var p = m.Va();
        m = p ? p.height / JA(m).height : 0;
        c == k ? (b.lineHeight = "90%", this.A.style.verticalAlign = "center" == l ? "middle" : "left" ==
            l ? d < e ? "top" : "bottom" : d < e ? "bottom" : "top", b.textAlign = "center", k = f.size * g, b.top = Math.round(Math.min(d, e) * m) + "px", b.left = Math.round((c - k / 2) * g) + "px", b.width = Math.round(k) + "px", b.height = Math.abs(d - e) * m + "px", b.fontSize = .6 * f.size * m + "pt") : (b.lineHeight = "100%", this.A.style.verticalAlign = "top", b.textAlign = l, b.top = Math.round(((d + e) / 2 - 2 * f.size / 3) * m) + "px", b.left = Math.round(c * g) + "px", b.width = Math.round(Math.abs(k - c) * g) + "px", b.height = "auto", b.fontSize = f.size * m + "pt");
        b.fontWeight = "normal";
        b.fontStyle = "normal";
        b.fontFamily =
            f.A;
        c = this.fill;
        b.color = c.bb() || c.cg();
        $A(this);
        a.Z().appendChild(n);
        n.appendChild(this.A)
    }
    H(ZA, UA);
    ZA.prototype.sh = function(a) {
        this.fill = a;
        var b = this.Z();
        b && (b.style.color = a.bb() || a.cg())
    };
    ZA.prototype.th = aa();
    ZA.prototype.B = aa();

    function $A(a) {
        if (a.D == a.F) {
            var b = ob(a.C.split(""), function(a) {
                    return Ma(a)
                }).join("<br>"),
                b = Si(ti("Concatenate escaped chars and <br>"), b);
            Gj(a.A, b)
        } else Gj(a.A, Li(a.C))
    }

    function aB(a, b, c, d, e, f, g) {
        PA.call(this, a, b);
        this.F = c;
        this.G = d;
        this.C = e;
        this.A = f;
        this.I = g
    }
    H(aB, SA);
    aB.prototype.B = function(a) {
        this.D ? this.C && this.A && a.drawImage(this.D, this.F, this.G, this.C, this.A) : (a = new Image, a.onload = E(this.H, this, a), a.src = this.I)
    };
    aB.prototype.H = function(a) {
        this.D = a;
        WA(this.fb)
    };

    function bB() {};

    function cB(a, b) {
        this.B = a;
        this.A = null == b ? 1 : b
    }
    H(cB, bB);
    cB.prototype.bb = h("B");

    function dB(a, b, c, d, e) {
        IA.call(this, a, b, c, d, e)
    }
    H(dB, IA);
    q = dB.prototype;
    q.Ze = function() {
        WA(this)
    };
    q.af = function() {
        WA(this)
    };
    q.ie = function() {
        WA(this)
    };

    function eB(a, b) {
        a = a.getContext();
        a.save();
        b = b.Ud ? OA(b.Ud) : new MA;
        var c = b.D,
            d = b.G;
        (c || d) && a.translate(c, d);
        (b = b.A) && a.rotate(Math.asin(b))
    }
    q.vc = function() {
        var a = this.G.He("DIV", {
            style: "position:relative;overflow:hidden"
        });
        this.D = a;
        this.C = this.G.He("CANVAS");
        a.appendChild(this.C);
        this.O = this.B = new VA(this);
        this.P = 0;
        fB(this)
    };
    q.getContext = function() {
        this.Z() || this.vc();
        this.A || (this.A = this.C.getContext("2d"), this.A.save());
        return this.A
    };
    q.gc = function(a, b) {
        dB.V.gc.apply(this, arguments);
        WA(this)
    };
    q.Va = function() {
        var a = this.width,
            b = this.height,
            c = wa(a) && -1 != a.indexOf("%"),
            d = wa(b) && -1 != b.indexOf("%");
        if (!this.Ka && (c || d)) return null;
        var e, f;
        c && (e = this.Z().parentNode, f = Yn(e), a = parseFloat(a) * f.width / 100);
        d && (e = e || this.Z().parentNode, f = f || Yn(e), b = parseFloat(b) * f.height / 100);
        return new Ej(a, b)
    };

    function fB(a) {
        Wn(a.Z(), a.width, a.height);
        var b = a.Va();
        b && (Wn(a.C, b.width, b.height), a.C.width = b.width, a.C.height = b.height, a.A = null)
    }
    q.reset = function() {
        var a = this.getContext();
        a.restore();
        var b = this.Va();
        b.width && b.height && a.clearRect(0, 0, b.width, b.height);
        a.save()
    };
    q.clear = function() {
        this.reset();
        this.B.clear();
        for (var a = this.Z(); 1 < a.childNodes.length;) a.removeChild(a.lastChild)
    };

    function WA(a) {
        if (!a.U && a.Ka) {
            a.reset();
            if (a.F) {
                var b = a.Va();
                a.getContext().scale(b.width / a.F, b.height / a.H)
            }
            eB(a, a.B);
            a.B.B(a.A);
            a.getContext().restore()
        }
    }

    function XA(a, b) {
        if (!(b instanceof ZA)) {
            var c = a.getContext();
            eB(a, b);
            if (b.Vi && b.Lj) {
                var d = b.fill;
                if (d)
                    if (d instanceof cB) 0 != d.A && (c.globalAlpha = d.A, c.fillStyle = d.bb(), b.B(c), c.fill(), c.globalAlpha = 1);
                    else {
                        var e = c.createLinearGradient(d.pn(), d.rn(), d.qn(), d.sn());
                        e.addColorStop(0, d.cg());
                        e.addColorStop(1, d.on());
                        c.fillStyle = e;
                        b.B(c);
                        c.fill()
                    }
                if (d = b.ff) b.B(c), c.strokeStyle = d.bb(), b = d.W(), wa(b) && -1 != b.indexOf("px") && (b = parseFloat(b) / KA(a)), c.lineWidth = b, c.stroke()
            } else b.B(c);
            a.getContext().restore()
        }
    }

    function gB(a, b, c) {
        c = c || a.B;
        c.appendChild(b);
        !a.Ka || a.P || c != a.B && c != a.O || XA(a, b)
    }
    q.drawImage = function(a, b, c, d, e, f) {
        a = new aB(null, this, a, b, c, d, e);
        gB(this, a, f);
        return a
    };
    q.Ee = function(a, b, c, d, e, f, g, k, l, m) {
        a = new ZA(this, a, b, c, d, e, f, g, k, l);
        gB(this, a, m);
        return a
    };
    q.De = function(a, b, c, d) {
        a = new YA(null, this, a, b, c);
        gB(this, a, d);
        return a
    };
    q.ea = function() {
        this.A = null;
        dB.V.ea.call(this)
    };
    q.Mb = function() {
        var a = this.Va();
        dB.V.Mb.call(this);
        a || (fB(this), this.dispatchEvent("resize"));
        WA(this)
    };

    function hB(a, b, c) {
        this.C = a;
        this.B = b;
        this.A = null == c ? 1 : c
    }
    hB.prototype.W = h("C");
    hB.prototype.bb = h("B");

    function iB(a, b) {
        PA.call(this, a, b)
    }
    H(iB, RA);
    iB.prototype.clear = function() {
        Sj(this.Z())
    };

    function jB(a, b, c, d) {
        QA.call(this, a, b, c, d)
    }
    H(jB, TA);
    jB.prototype.A = function(a) {
        kB(this.Z(), {
            d: lB(a)
        })
    };

    function mB(a, b, c, d) {
        QA.call(this, a, b, c, d)
    }
    H(mB, UA);

    function nB(a, b) {
        PA.call(this, a, b)
    }
    H(nB, SA);

    function oB(a, b, c, d, e) {
        IA.call(this, a, b, c, d, e);
        this.P = {};
        this.O = Uc && !ed(526);
        this.A = new wq(this)
    }
    var pB;
    H(oB, IA);

    function qB(a, b, c) {
        a = a.G.A.createElementNS("http://www.w3.org/2000/svg", b);
        c && kB(a, c);
        return a
    }

    function kB(a, b) {
        for (var c in b) a.setAttribute(c, b[c])
    }
    q = oB.prototype;
    q.Ze = function(a, b) {
        a = a.Z();
        b instanceof cB ? (a.setAttribute("fill", b.bb()), a.setAttribute("fill-opacity", b.A)) : a.setAttribute("fill", "none")
    };
    q.af = function(a, b) {
        a = a.Z();
        b ? (a.setAttribute("stroke", b.bb()), a.setAttribute("stroke-opacity", b.A), b = b.W(), wa(b) && -1 != b.indexOf("px") ? a.setAttribute("stroke-width", parseFloat(b) / KA(this)) : a.setAttribute("stroke-width", b)) : a.setAttribute("stroke", "none")
    };
    q.ie = function(a, b) {
        b = [b.B, b.A, b.C, b.F, b.D, b.G].join();
        a.Z().setAttribute("transform", "matrix(" + b + ")")
    };
    q.vc = function() {
        var a = qB(this, "svg", {
                width: this.width,
                height: this.height,
                overflow: "hidden"
            }),
            b = qB(this, "g");
        this.C = qB(this, "defs");
        this.B = new iB(b, this);
        a.appendChild(this.C);
        a.appendChild(b);
        this.D = a;
        rB(this)
    };
    q.gc = function(a, b) {
        oB.V.gc.apply(this, arguments);
        rB(this)
    };

    function rB(a) {
        a.F && (a.Z().setAttribute("preserveAspectRatio", "none"), a.O ? a.oe() : a.Z().setAttribute("viewBox", "0 0 " + (a.F ? a.F + " " + a.H : "")))
    }
    q.oe = function() {
        if (this.Ka) {
            var a = this.Va();
            if (0 == a.width) this.Z().style.visibility = "hidden";
            else {
                this.Z().style.visibility = "";
                var b = a.width / this.F,
                    a = a.height / this.H;
                this.B.Z().setAttribute("transform", "scale(" + b + " " + a + ") translate(0 0)")
            }
        }
    };
    q.Va = function() {
        if (!Tc) return this.Ka ? Yn(this.Z()) : oB.V.Va.call(this);
        var a = this.width,
            b = this.height,
            c = wa(a) && -1 != a.indexOf("%"),
            d = wa(b) && -1 != b.indexOf("%");
        if (!this.Ka && (c || d)) return null;
        var e, f;
        c && (e = this.Z().parentNode, f = Yn(e), a = parseFloat(a) * f.width / 100);
        d && (e = e || this.Z().parentNode, f = f || Yn(e), b = parseFloat(b) * f.height / 100);
        return new Ej(a, b)
    };
    q.clear = function() {
        this.B.clear();
        Sj(this.C);
        this.P = {}
    };
    q.drawImage = function(a, b, c, d, e, f) {
        a = qB(this, "image", {
            x: a,
            y: b,
            width: c,
            height: d,
            "image-rendering": "optimizeQuality",
            preserveAspectRatio: "none"
        });
        a.setAttributeNS("http://www.w3.org/1999/xlink", "href", e);
        e = new nB(a, this);
        (f || this.B).Z().appendChild(e.Z());
        return e
    };
    q.Ee = function(a, b, c, d, e, f, g, k, l, m) {
        var n = Math.round(Ib(Kb(Math.atan2(e - c, d - b))));
        d -= b;
        e -= c;
        e = Math.round(Math.sqrt(d * d + e * e));
        d = g.size;
        g = {
            "font-family": g.A,
            "font-size": d
        };
        d = Math.round(c - d / 2 + Math.round(.85 * d));
        var p = b;
        "center" == f ? (p += Math.round(e / 2), g["text-anchor"] = "middle") : "right" == f && (p += e, g["text-anchor"] = "end");
        g.x = p;
        g.y = d;
        0 != n && (g.transform = "rotate(" + n + " " + b + " " + c + ")");
        b = qB(this, "text", g);
        b.appendChild(this.G.A.createTextNode(a));
        null == k && Tc && Wc && (a = "black", l instanceof cB && (a = l.bb()), k = new hB(1,
            a));
        l = new mB(b, this, k, l);
        (m || this.B).Z().appendChild(l.Z());
        return l
    };
    q.De = function(a, b, c, d) {
        a = qB(this, "path", {
            d: lB(a)
        });
        b = new jB(a, this, b, c);
        (d || this.B).Z().appendChild(b.Z());
        return b
    };

    function lB(a) {
        var b = [];
        EA(a, function(a, d) {
            switch (a) {
                case 0:
                    b.push("M");
                    Array.prototype.push.apply(b, d);
                    break;
                case 1:
                    b.push("L");
                    Array.prototype.push.apply(b, d);
                    break;
                case 2:
                    b.push("C");
                    Array.prototype.push.apply(b, d);
                    break;
                case 3:
                    a = d[3];
                    b.push("A", d[0], d[1], 0, 180 < Math.abs(a) ? 1 : 0, 0 < a ? 1 : 0, d[4], d[5]);
                    break;
                case 4:
                    b.push("Z")
            }
        });
        return b.join(" ")
    }
    q.Mb = function() {
        var a = this.Va();
        oB.V.Mb.call(this);
        a || this.dispatchEvent("resize");
        if (this.O) {
            var a = this.width,
                b = this.height;
            "string" == typeof a && -1 != a.indexOf("%") && "string" == typeof b && -1 != b.indexOf("%") && this.A.listen(sB(), "tick", this.oe);
            this.oe()
        }
    };
    q.Mc = function() {
        oB.V.Mc.call(this);
        this.O && this.A.Ub(sB(), "tick", this.oe)
    };
    q.ea = function() {
        delete this.P;
        delete this.C;
        delete this.B;
        this.A.ra();
        delete this.A;
        oB.V.ea.call(this)
    };

    function sB() {
        pB || (pB = new qv(400), pB.start());
        return pB
    };

    function tB() {
        return this.Ob = this.fb.G.Z(this.ta) || this.Ob
    }

    function uB(a, b) {
        this.ta = a.id;
        PA.call(this, a, b)
    }
    H(uB, RA);
    uB.prototype.Z = tB;
    uB.prototype.clear = function() {
        Sj(this.Z())
    };

    function vB(a, b, c, d) {
        this.ta = a.id;
        QA.call(this, a, b, c, d)
    }
    H(vB, TA);
    vB.prototype.Z = tB;
    vB.prototype.A = function(a) {
        wB(this.Z(), "path", xB(a))
    };

    function yB(a, b, c, d) {
        this.ta = a.id;
        QA.call(this, a, b, c, d)
    }
    H(yB, UA);
    yB.prototype.Z = tB;

    function zB(a, b) {
        this.ta = a.id;
        PA.call(this, a, b)
    }
    H(zB, SA);
    zB.prototype.Z = tB;

    function AB(a, b, c, d, e) {
        IA.call(this, a, b, c, d, e);
        this.A = new wq(this);
        Ck(this, Fa(Dk, this.A))
    }
    H(AB, IA);
    var BB = w.document && w.document.documentMode && 8 <= w.document.documentMode;

    function CB(a) {
        return wa(a) && Ja(a, "%") ? a : parseFloat(a.toString()) + "px"
    }

    function DB(a) {
        return Math.round(100 * (parseFloat(a.toString()) - .5))
    }

    function EB(a) {
        return Math.round(100 * parseFloat(a.toString()))
    }

    function wB(a, b, c) {
        BB ? a[b] = c : a.setAttribute(b, c)
    }

    function FB(a, b) {
        a = a.G.A.createElement(String("g_vml_:" + b));
        a.id = "goog_" + fb++;
        return a
    }

    function GB(a) {
        if (BB && a.Ka) {
            var b = Si(ti("Assign innerHTML to itself"), a.Z().innerHTML);
            Gj(a.Z(), b)
        }
    }

    function HB(a, b, c) {
        (c || a.B).Z().appendChild(b.Z());
        GB(a)
    }
    AB.prototype.Ze = function(a, b) {
        a = a.Z();
        IB(a);
        if (b instanceof cB)
            if ("transparent" == b.bb()) a.filled = !1;
            else if (1 != b.A) {
            a.filled = !0;
            var c = FB(this, "fill");
            c.opacity = Math.round(100 * b.A) + "%";
            c.color = b.bb();
            a.appendChild(c)
        } else a.filled = !0, a.fillcolor = b.bb();
        else a.filled = !1;
        GB(this)
    };
    AB.prototype.af = function(a, b) {
        a = a.Z();
        if (b) {
            a.stroked = !0;
            var c = b.W(),
                c = wa(c) && -1 == c.indexOf("px") ? parseFloat(c) : c * KA(this),
                d = a.getElementsByTagName("stroke")[0];
            d || (d = d || FB(this, "stroke"), a.appendChild(d));
            d.opacity = b.A;
            d.weight = c + "px";
            d.color = b.bb()
        } else a.stroked = !1;
        GB(this)
    };
    AB.prototype.ie = function(a, b) {
        a = a.Z();
        JB(a);
        var c = FB(this, "skew");
        c.xn = "true";
        c.origin = -a.style.pixelLeft / a.style.pixelWidth - .5 + "," + (-a.style.pixelTop / a.style.pixelHeight - .5);
        c.offset = b.D.toFixed(1) + "px," + b.G.toFixed(1) + "px";
        c.vn = [b.B.toFixed(6), b.C.toFixed(6), b.A.toFixed(6), b.F.toFixed(6), 0, 0].join();
        a.appendChild(c);
        GB(this)
    };

    function JB(a) {
        nb(a.childNodes, function(b) {
            "skew" == b.tagName && a.removeChild(b)
        })
    }

    function IB(a) {
        a.fillcolor = "";
        nb(a.childNodes, function(b) {
            "fill" == b.tagName && a.removeChild(b)
        })
    }

    function KB(a, b, c, d, e) {
        var f = a.style;
        f.position = "absolute";
        f.left = DB(b) + "px";
        f.top = DB(c) + "px";
        f.width = EB(d) + "px";
        f.height = EB(e) + "px";
        "shape" == a.tagName && (a.coordsize = EB(d) + " " + EB(e))
    }

    function LB(a) {
        var b = FB(a, "shape");
        a = JA(a);
        KB(b, 0, 0, a.width, a.height);
        return b
    }
    if (Rc) try {
        Tb(document.namespaces)
    } catch (a) {}
    q = AB.prototype;
    q.vc = function() {
        var a = this.G.A;
        a.namespaces.g_vml_ || (BB ? a.namespaces.add("g_vml_", "urn:schemas-microsoft-com:vml", "#default#VML") : a.namespaces.add("g_vml_", "urn:schemas-microsoft-com:vml"), a.createStyleSheet().cssText = "g_vml_\\:*{behavior:url(#default#VML)}");
        var a = this.width,
            b = this.height,
            c = this.G.He("DIV", {
                style: "overflow:hidden;position:relative;width:" + CB(a) + ";height:" + CB(b)
            });
        this.D = c;
        var d = FB(this, "group"),
            e = d.style;
        e.position = "absolute";
        e.left = e.top = "0";
        e.width = this.width;
        e.height = this.height;
        d.coordsize = this.F ? EB(this.F) + " " + EB(this.H) : EB(a) + " " + EB(b);
        d.coordorigin = y(0) ? EB(0) + " " + EB(0) : "0 0";
        c.appendChild(d);
        this.B = new uB(d, this);
        Vk(c, "resize", E(this.Ie, this))
    };
    q.Ie = function() {
        var a = Yn(this.Z()),
            b = this.B.Z().style;
        if (a.width) b.width = a.width + "px", b.height = a.height + "px";
        else {
            for (a = this.Z(); a && a.currentStyle && "none" != a.currentStyle.display;) a = a.parentNode;
            a && a.currentStyle && this.A.listen(a, "propertychange", this.Ie)
        }
        this.dispatchEvent("resize")
    };
    q.gc = function(a, b) {
        AB.V.gc.apply(this, arguments);
        this.B.Z().coordsize = EB(a) + " " + EB(b)
    };
    q.Va = function() {
        var a = this.Z();
        return new Ej(a.style.pixelWidth || a.offsetWidth || 1, a.style.pixelHeight || a.offsetHeight || 1)
    };
    q.clear = function() {
        this.B.clear()
    };
    q.drawImage = function(a, b, c, d, e, f) {
        var g = FB(this, "image");
        KB(g, a, b, c, d);
        wB(g, "src", e);
        a = new zB(g, this);
        HB(this, a, f);
        return a
    };
    q.Ee = function(a, b, c, d, e, f, g, k, l, m) {
        var n = LB(this),
            p = FB(this, "path");
        wB(p, "v", "M" + DB(b) + "," + DB(c) + "L" + DB(d) + "," + DB(e) + "E");
        wB(p, "textpathok", "true");
        b = FB(this, "textpath");
        b.setAttribute("on", "true");
        c = b.style;
        c.fontSize = g.size * KA(this);
        c.fontFamily = g.A;
        null != f && (c["v-text-align"] = f);
        wB(b, "string", a);
        n.appendChild(p);
        n.appendChild(b);
        a = new yB(n, this, k, l);
        HB(this, a, m);
        return a
    };
    q.De = function(a, b, c, d) {
        var e = LB(this);
        wB(e, "path", xB(a));
        a = new vB(e, this, b, c);
        HB(this, a, d);
        return a
    };

    function xB(a) {
        var b = [];
        EA(a, function(a, d) {
            switch (a) {
                case 0:
                    b.push("m");
                    Array.prototype.push.apply(b, ob(d, EB));
                    break;
                case 1:
                    b.push("l");
                    Array.prototype.push.apply(b, ob(d, EB));
                    break;
                case 2:
                    b.push("c");
                    Array.prototype.push.apply(b, ob(d, EB));
                    break;
                case 4:
                    b.push("x");
                    break;
                case 3:
                    a = d[2] + d[3], b.push("ae", EB(d[4] - d[0] * Math.cos(Jb(a))), EB(d[5] - d[1] * Math.sin(Jb(a))), EB(d[0]), EB(d[1]), Math.round(-65536 * d[2]), Math.round(-65536 * d[3]))
            }
        });
        return b.join(" ")
    }
    q.Mb = function() {
        AB.V.Mb.call(this);
        this.Ie();
        GB(this)
    };
    q.ea = function() {
        this.B = null;
        AB.V.ea.call(this)
    };

    function MB(a) {
        this.B = a;
        this.A = Array(a);
        for (var b = 0; b < a; b++) this.A[b] = 0
    }

    function NB() {}

    function OB(a, b, c) {
        if (a instanceof MB)
            for (this.length = c || a.B / this.A, this.B = new MB(a.B), b = 0; b < this.length; b++) this[b] = a.A[b];
        else {
            if (va(a)) {
                for (b = 0; b < a.length; b++) this[b] = a[b];
                this.length = a.length
            } else
                for (this.length = a || 0, b = 0; b < this.length; b++) this[b] = 0;
            this.B = new MB(this.length * this.A)
        }
        this.B.A = this
    }
    H(OB, NB);
    OB.prototype.set = function(a, b) {
        b = b || 0;
        for (var c = 0; c < a.length; c++) this[b + c] = a[c]
    };
    OB.prototype.slice = aa();
    OB.prototype.subarray = ca(null);

    function PB(a, b, c) {
        OB.call(this, a, 0, c)
    }
    H(PB, OB);
    PB.prototype.A = 1;

    function QB(a, b, c) {
        OB.call(this, a, 0, c)
    }
    H(QB, OB);
    QB.prototype.A = 1;

    function RB(a, b, c) {
        OB.call(this, a, 0, c)
    }
    H(RB, OB);
    RB.prototype.A = 2;

    function SB(a, b, c) {
        OB.call(this, a, 0, c)
    }
    H(SB, OB);
    SB.prototype.A = 2;

    function TB(a, b, c) {
        OB.call(this, a, 0, c)
    }
    H(TB, OB);
    TB.prototype.A = 4;

    function UB(a, b, c) {
        OB.call(this, a, 0, c)
    }
    H(UB, OB);
    UB.prototype.A = 4;

    function VB(a, b, c) {
        OB.call(this, a, 0, c)
    }
    H(VB, OB);
    VB.prototype.A = 4;

    function WB(a, b, c) {
        OB.call(this, a, 0, c)
    }
    H(WB, OB);
    WB.prototype.A = 4;

    function XB() {}
    H(XB, NB);
    "undefined" == typeof ArrayBuffer && (w.ArrayBuffer = MB);
    "undefined" == typeof Int8Array && (w.Int8Array = PB);
    "undefined" == typeof Uint8Array && (w.Uint8Array = QB);
    "undefined" == typeof Int16Array && (w.Int16Array = RB);
    "undefined" == typeof Uint16Array && (w.Uint16Array = SB);
    "undefined" == typeof Int32Array && (w.Int32Array = TB);
    "undefined" == typeof Uint32Array && (w.Uint32Array = UB);
    "undefined" == typeof Float32Array && (w.Float32Array = VB);
    "undefined" == typeof Float64Array && (w.Float64Array = WB);
    "undefined" == typeof DataView && (w.DataView = XB);

    function YB(a) {
        return a.Sa ? 1 : a.yb ? 2 : 3
    }

    function ZB(a, b, c, d, e) {
        var f = a.A,
            g = $B(b, c),
            k = Math.max(d * g, 1),
            g = Math.max(e * g, 1);
        Jd || Rc ? (k = Math.round(k), g = Math.round(g)) : (k = Math.floor(k), g = Math.floor(g));
        if (f.width !== k || f.height !== g || a.C !== c) a.C = c, f.width = k, f.height = g, Kd && 1 == b ? (a = 1 / c, f.style.transformOrigin = "0 0", f.style.webkitTransformOrigin = "0 0", f.style.transform = "scale(" + a + "," + a + ")", f.style.webkitTransform = "scale(" + a + "," + a + ")") : (f.style.width = d + "px", f.style.height = e + "px")
    }

    function $B(a, b) {
        2 == a && Kd ? (0 >= aC && (aC = Rj("canvas").getContext("2d").webkitBackingStorePixelRatio || 1), a = b / aC) : a = b;
        return a
    }
    var aC = -1;

    function bC(a) {
        this.A = [];
        this.K = El();
        this.I = El();
        this.O = El();
        this.C = this.H = this.P = this.F = this.J = null;
        this.G = [];
        this.B = [];
        this.ha = this.ja = this.ga = this.U = null;
        this.D = 1;
        this.Ma = !!a;
        this.Y = Infinity;
        this.aa = this.da = -1;
        a = cC("rgba(255, 255, 255, 0.7)");
        var b = cC("rgba(0, 0, 0, 0.15)"),
            c = cC("rgba(0, 0, 0, 0.5)");
        this.ga = new cB(dC(a), a[3]);
        this.ja = new hB(2, dC(b), b[3]);
        this.U = new BA;
        this.ha = new cB(dC(c), c[3]);
        this.$ = El();
        this.M = El();
        this.wa = ik();
        this.xa = ik();
        this.na = ok();
        this.N = rl();
        this.Ga = rl();
        this.Ua = rl();
        this.L = new Float64Array(2);
        this.X = Cl();
        this.Wa = Cl();
        this.Ta = new CA
    }

    function eC(a, b) {
        for (var c = 0; c < b.length; c++) a.add(b[c])
    }
    bC.prototype.add = function(a) {
        this.A.push(a);
        Rz(a)
    };

    function fC(a, b, c, d) {
        var e;
        if (c.yb) e = c.yb.canvas;
        else if (c.Sa) e = c.Sa.C.A;
        else if (c.A) e = c.A;
        else throw Error("ShapeLayer: invalid context: " + c);
        var f = w.devicePixelRatio || 1,
            g = b.M,
            k = b.K,
            l = e.height / f / 2,
            m = e.width / f / 2,
            n = a.O,
            p = -l,
            r = (k - g) / 2,
            u = (k + g) / 2;
        n[0] = m;
        n[1] = 0;
        n[2] = 0;
        n[3] = 0;
        n[4] = 0;
        n[5] = p;
        n[6] = 0;
        n[7] = 0;
        n[8] = 0;
        n[9] = 0;
        n[10] = r;
        n[11] = 0;
        n[12] = m;
        n[13] = l;
        n[14] = u;
        n[15] = 1;
        var t = a.N,
            v = a.Ga,
            x = a.Ua;
        sl(t, b.A, b.B, b.C);
        sl(v, b.D, b.F, b.G);
        Yl(t, t);
        Yl(v, v);
        yl(t, x);
        Rl(a.I, t, v, x);
        var D = b.W() / b.H,
            z = a.K,
            A = b.M,
            B = b.K,
            Q = b.J / 2,
            M = B - A,
            G = Math.sin(Q);
        if (0 != M && 0 != G && 0 != D) {
            var T = Math.cos(Q) / G;
            z[0] = T / D;
            z[1] = 0;
            z[2] = 0;
            z[3] = 0;
            z[4] = 0;
            z[5] = T;
            z[6] = 0;
            z[7] = 0;
            z[8] = 0;
            z[9] = 0;
            z[10] = -(B + A) / M;
            z[11] = -1;
            z[12] = 0;
            z[13] = 0;
            z[14] = -(2 * A * B) / M;
            z[15] = 0
        }
        Ml(a.K, a.I, a.$);
        if (c.yb) {
            for (var R = c.yb, qa = $B(2, w.devicePixelRatio || 1), na = a.M, sa = 0; sa < a.A.length; sa++) {
                var da = a.A[sa];
                if (!hz(da)) {
                    Ml(a.K, a.I, na);
                    Ml(na, iz(da), na);
                    var ea;
                    a: {
                        for (var gb = da.A, ib = na, Hb = a.X, sc = 0; sc < gb.length; sc += 3)
                            if (Hb[0] = gb[sc + 0], Hb[1] = gb[sc + 1], Hb[2] = gb[sc + 2], Hb[3] = 1, Ql(ib, Hb, Hb), Zz(Hb) & 1) {
                                ea = !1;
                                break a
                            }
                        ea = !0
                    }
                    if (ea) {
                        var Ra = a.X;
                        R.beginPath();
                        for (var Ud = 0; Ud < da.A.length / 3; Ud++) Ra[0] = da.A[3 * Ud + 0], Ra[1] = da.A[3 * Ud + 1], Ra[2] = da.A[3 * Ud + 2], Ra[3] = 1, Ql(na, Ra, Ra), wl(Ra, 1 / Ra[3], Ra), Ol(a.O, Ra, Ra), wl(Ra, qa, Ra), 0 == Ud ? R.moveTo(Ra[0], Ra[1]) : R.lineTo(Ra[0], Ra[1]);
                        R.closePath();
                        var Vi = gC;
                        R.fillStyle = Vi(da.D);
                        R.fill();
                        var Wi = da.C;
                        Wi && (R.strokeStyle = Vi(Wi), R.stroke())
                    }
                }
            }
            if (0 != a.B.length && 0 != a.D) {
                R.globalAlpha = a.D;
                var Xi = a.M;
                Ml(a.K, a.I, Xi);
                for (var Bt = 0; Bt < a.B.length; Bt++) {
                    var NC = a.B[Bt],
                        En = NC.Ja(),
                        kg = a.X,
                        Ct =
                        NC.Pc(),
                        OC = a.Wa,
                        Fn = OC,
                        gM = Ct[1],
                        hM = Ct[2];
                    Fn[0] = Ct[0];
                    Fn[1] = gM;
                    Fn[2] = hM;
                    Fn[3] = 1;
                    var iM = a,
                        lg = kg;
                    Ql(Xi, OC, lg);
                    var PC = !1;
                    0 != (Zz(lg) & 6) && (PC = !0);
                    wl(lg, 1 / lg[3], lg);
                    Ol(iM.O, lg, lg);
                    if (!PC && En) {
                        var QC = qa * En.width,
                            RC = qa * En.height;
                        kg[0] = kg[0] * qa - QC / 2;
                        kg[1] = kg[1] * qa - RC / 2;
                        R.drawImage(En, kg[0], kg[1], QC, RC)
                    }
                }
                R.globalAlpha = 1
            }
        } else if (c.Sa) {
            var Ka = c.Sa;
            Ka.clear(256);
            Ka.disable(2884);
            Ka.disable(2929);
            Ka.enable(3042);
            Ka.blendFuncSeparate(770, 771, 1, 771);
            if (!a.J) {
                var Dt = new uA,
                    SC = a.Ma;
                Dt.A != SC && (Dt.A = +SC);
                a.J = new tA(Ka,
                    Dt)
            }
            fA(a.J);
            var jM = aA(a.J.attributes.A);
            Ka.enableVertexAttribArray(jM);
            for (var Et = 0; Et < a.A.length; Et++) {
                var Ft = a.A[Et],
                    Gt = Ka;
                Gt.B.contains(Ft.G) ? Yz(Gt.B, Ft.G) : Vz(Ft, Gt)
            }
            for (var Gn = a.M, Ht = 0; Ht < a.A.length; Ht++) {
                var mg = a.A[Ht];
                if (!hz(mg)) {
                    var Ch = a.J;
                    Ml(a.K, a.I, Gn);
                    Ml(Gn, iz(mg), Gn);
                    var TC = a.na;
                    pk(TC, Gn);
                    pA(Ch.B.A, TC);
                    nA(Ch.B.color, mg.D);
                    Ka.bindBuffer(34962, Xz(mg, Ka));
                    Ch.attributes.A.vertexAttribPointer(3, 5126, !1, 0, 0); - 1 != aA(Ch.attributes.B) && Ch.B.B.set(0);
                    Ka.drawArrays(6, 0, mg.A.length / 3);
                    var UC = mg.C;
                    UC && (nA(Ch.B.color, UC), Ka.drawArrays(2, 0, mg.A.length / 3))
                }
            }
            if (0 != a.B.length) {
                a.F || (a.F = new qA(Ka));
                fA(a.F);
                var je = a.N;
                sl(je, b.A, b.B, b.C);
                Yl(je, je);
                var It = a.M;
                Ml(a.K, a.I, It);
                Tl(It, je[0], je[1], je[2]);
                var VC = a.na;
                pk(VC, It);
                pA(a.F.B.B, VC);
                a.F.B.opacity.set(a.D);
                a.P ? Ka.bindBuffer(34962, a.P) : (a.P = Ka.createBuffer(), Ka.bindBuffer(34962, a.P), Ka.bufferData(34962, new Float32Array([-1, -1, 1, -1, -1, 1, 1, 1]), 35044));
                var Dh = a.F;
                Ka.vertexAttribPointer(aA(Dh.attributes.A), 2, 5126, !1, 0, 0);
                Ka.enableVertexAttribArray(aA(Dh.attributes.A));
                Dh.B.A.set(0);
                for (var Jt = 0; Jt < a.B.length; Jt++) {
                    var Kt = a.B[Jt],
                        Lt = Kt.Pc(),
                        WC = Kt.Ja(),
                        XC = Kt.Aa(Ka);
                    XC && (Ka.uniform3fv(hA(Dh.B.D), [Lt[0] - je[0], Lt[1] - je[1], Lt[2] - je[2]]), Ka.uniform2fv(hA(Dh.B.C), [WC.width / b.W(), WC.height / b.H]), Ka.activeTexture(33984), Ka.bindTexture(3553, XC), Ka.drawArrays(5, 0, 4))
                }
                Ka.disableVertexAttribArray(aA(Dh.attributes.A))
            }
            var kM = aA(a.J.attributes.A);
            Ka.disableVertexAttribArray(kM);
            Ka.disable(3042)
        } else if (c.A && d) {
            var wc = c.A,
                Eh = a.N,
                Fh = a.Ga;
            sl(Eh, b.A, b.B, b.C);
            sl(Fh, b.D, b.F, b.G);
            Yl(Eh,
                Eh);
            Yl(Fh, Fh);
            wl(Eh, 2, Eh);
            vl(Eh, Fh, Fh);
            var Gh = a.L;
            d(Fh, Gh);
            var YC = 2 * Math.abs(b.W() / 2 - Gh[0]);
            Gh[0] < b.W() / 2 && (Gh[0] += YC);
            a.Y = Gh[0] - .1 * YC;
            if (0 !== a.A.length || 0 !== a.G.length || 0 !== a.B.length) {
                var Uj;
                a.C || (a.C = Rj("canvas"), a.C.style.position = "absolute", a.C.style.zIndex = "2", wc.appendChild(a.C));
                if (a.C.width != wc.clientWidth || a.C.height != wc.clientHeight) a.C.width = wc.clientWidth, a.C.height = wc.clientHeight;
                var cb = (Uj = a.C) && Uj.getContext ? Uj.getContext("2d") : null;
                if (cb) {
                    cb.save();
                    cb.setTransform(1, 0, 0, 1, 0, 0);
                    cb.clearRect(0,
                        0, Uj.width, Uj.height);
                    cb.restore();
                    for (var Mt = 0; Mt < a.G.length; Mt++) {
                        var ZC = a.G[Mt],
                            ng = a.wa;
                        hC(a, ZC, ng, d) && (cb.save(), cb.transform(ng[0], ng[1], ng[3], ng[4], ng[6], ng[7]), cb.drawImage(ZC.Aa(), 0, 0), cb.restore())
                    }
                    cb.save();
                    cb.setTransform(1, 0, 0, 1, 0, 0);
                    for (var Nt = 0; Nt < a.A.length; Nt++) {
                        var Hn = a.A[Nt];
                        if (!hz(Hn)) {
                            var Hh = [];
                            if (iC(a, Hn, Hh, d)) {
                                cb.beginPath();
                                cb.moveTo(Hh[0], Hh[1]);
                                for (var In = 1; In < Hh.length / 2; In++) cb.lineTo(Hh[2 * In], Hh[2 * In + 1]);
                                cb.closePath();
                                var bD = gC;
                                cb.fillStyle = bD(Hn.D);
                                cb.fill();
                                var cD = Hn.C;
                                cD && (cb.strokeStyle = bD(cD), cb.stroke())
                            }
                        }
                    }
                    cb.restore();
                    if (0 != a.D) {
                        cb.globalAlpha = a.D;
                        for (var Ot = 0; Ot < a.B.length; Ot++) {
                            var dD = a.B[Ot],
                                og = a.L;
                            d(dD.Pc(), og);
                            var Ih = dD.Ja();
                            Ih && (og[0] -= Ih.width / 2, og[1] -= Ih.height / 2, cb.drawImage(Ih, og[0], og[1], Ih.width, Ih.height))
                        }
                        cb.globalAlpha = 1
                    }
                } else {
                    var Jh;
                    if (!a.H) {
                        var Vj;
                        !Rc || ed("9") && Hj().A.createElementNS ? !Uc || ed("420") && !Vc ? Vj = new oB("100%", "100%", void 0, void 0, void 0) : Vj = new dB("100%", "100%", void 0, void 0, void 0) : Vj = new AB("100%", "100%", void 0, void 0, void 0);
                        Vj.vc();
                        a.H = Vj;
                        var Pt = a.H.Z();
                        Pt.style.position = "absolute";
                        Pt.style.zIndex = "1";
                        Pt.setAttribute("pointer-events", "none");
                        var ke = a.H;
                        if (ke.Ka) throw Error("Component already rendered");
                        ke.D || ke.vc();
                        wc ? wc.insertBefore(ke.D, null) : ke.G.A.body.appendChild(ke.D);
                        ke.I && !ke.I.Ka || ke.Mb()
                    }
                    if (a.da != wc.clientWidth || a.aa != wc.clientHeight) a.H.gc(wc.clientWidth, wc.clientHeight), a.da = wc.clientWidth, a.aa = wc.clientHeight;
                    Jh = a.H;
                    Jh.clear();
                    for (var Qt = 0; Qt < a.G.length; Qt++) {
                        var Rt = a.G[Qt],
                            xc = a.wa;
                        if (hC(a, Rt, xc, d))
                            for (var lM =
                                    new MA(xc[0], xc[1], xc[3], xc[4], xc[6], xc[7]), mM = new MA(xc[0], xc[1], xc[3], xc[4], xc[6] + -1.5, xc[7] + 0), eD = Rt.Aa().height / 1.25, Wj = Rt.H, Kh = 0; Kh < Wj.length; ++Kh) {
                                var St = LA(Jh, Wj[Kh], eD / Wj.length * Kh, a.U, null, a.ha),
                                    fD = mM;
                                St.Ud = OA(fD);
                                St.fb.ie(St, fD);
                                var Tt = LA(Jh, Wj[Kh], eD / Wj.length * Kh, a.U, a.ja, a.ga),
                                    gD = lM;
                                Tt.Ud = OA(gD);
                                Tt.fb.ie(Tt, gD)
                            }
                    }
                    for (var Ut = 0; Ut < a.A.length; Ut++) {
                        var Jn = a.A[Ut];
                        if (!hz(Jn)) {
                            var Lh = [];
                            if (iC(a, Jn, Lh, d)) {
                                var Xj = a.Ta;
                                Xj.clear();
                                Xj.vg(Lh[0], Lh[1]);
                                for (var Kn = 1; Kn < Lh.length / 2; Kn++) Xj.ug(Lh[2 * Kn],
                                    Lh[2 * Kn + 1]);
                                Xj.close();
                                var hD = dC,
                                    iD = Jn.D,
                                    nM = new cB(hD(iD), iD[3]),
                                    Vt = Jn.C,
                                    jD = null;
                                if (Vt) var oM = hD(Vt),
                                    jD = new hB(1, oM, Vt[3]);
                                Jh.De(Xj, jD, nM)
                            }
                        }
                    }
                    if (0 != a.D)
                        for (var Wt = 0; Wt < a.B.length; Wt++) {
                            var kD = a.B[Wt],
                                pg = a.L;
                            d(kD.Pc(), pg);
                            var Mh = kD.Ja();
                            Mh && (pg[0] -= Mh.width / 2, pg[1] -= Mh.height / 2, Jh.drawImage(pg[0], pg[1], Mh.width, Mh.height, Mh.src))
                        }
                }
            }
        } else throw Error("ShapeLayer: invalid context: " + c);
    }

    function gC(a) {
        return "rgba(" + Math.floor(255 * a[0]) + "," + Math.floor(255 * a[1]) + "," + Math.floor(255 * a[2]) + "," + a[3] + ")"
    }

    function cC(a) {
        a = a.substring(5, a.length - 1).split(",");
        for (var b = [], c = 0; c < a.length; c++) b.push(+a[c]);
        for (c = 0; 3 > c; c++) b[c] /= 255;
        return b
    }

    function jC(a) {
        a = a.toString(16);
        return 1 == a.length ? "0" + a : a
    }

    function dC(a) {
        return "#" + jC(Math.floor(255 * a[0])) + jC(Math.floor(255 * a[1])) + jC(Math.floor(255 * a[2]))
    }

    function iC(a, b, c, d) {
        var e = a.Y;
        0 != c.length && (c = []);
        for (var f = !1, g = 0; g < b.A.length / 3; g++) {
            var k = a.N;
            k[0] = b.A[3 * g + 0];
            k[1] = b.A[3 * g + 1];
            k[2] = b.A[3 * g + 2];
            Ol(iz(b), k, k);
            var l = a.L;
            d(k, l);
            c.push(l[0]);
            c.push(l[1]);
            f = l[0] > e || f
        }
        return !f
    }

    function hC(a, b, c, d) {
        var e = a.Y,
            f = b.ma(),
            g = a.M;
        Np(f, g);
        for (var k = a.xa, l = 6, m = b.A.length; l < m; l++) k[l - 3] = b.A[l];
        for (l = 0; 3 > l; l++) k[l] = (b.A[l] + b.A[l + 3]) / 2;
        f = !1;
        l = 0;
        for (m = k.length / 3; l < m; l++) {
            var n = a.N;
            n[0] = k[3 * l + 0];
            n[1] = k[3 * l + 1];
            n[2] = k[3 * l + 2];
            Ol(g, n, n);
            Yl(n, n);
            var p = a.L;
            d(n, p);
            c[3 * l + 0] = p[0];
            c[3 * l + 1] = p[1];
            c[3 * l + 2] = 1;
            f = p[0] > e || f
        }
        d = b.Aa();
        b = d.height / 1.25;
        d = d.width;
        a = a.xa;
        a[0] = 0;
        a[1] = 0 + b / 2;
        a[2] = 1;
        a[3] = 0 + d;
        a[4] = 0 + b;
        a[5] = 1;
        a[6] = 0 + d;
        a[7] = 0;
        a[8] = 1;
        jk(a, a);
        b = c[0];
        d = c[1];
        var e = c[2],
            g = c[3],
            k = c[4],
            l = c[5],
            m = c[6],
            n = c[7],
            p = c[8],
            r = a[0],
            u = a[1],
            t = a[2],
            v = a[3],
            x = a[4],
            D = a[5],
            z = a[6],
            A = a[7];
        a = a[8];
        c[0] = b * r + g * u + m * t;
        c[1] = d * r + k * u + n * t;
        c[2] = e * r + l * u + p * t;
        c[3] = b * v + g * x + m * D;
        c[4] = d * v + k * x + n * D;
        c[5] = e * v + l * x + p * D;
        c[6] = b * z + g * A + m * a;
        c[7] = d * z + k * A + n * a;
        c[8] = e * z + l * A + p * a;
        return !f
    };

    function kC(a, b) {
        this.G = a;
        this.B = new zm;
        this.A = b;
        this.F = new bC;
        this.D = null;
        this.H = E(this.G.Tc, this.G);
        this.I = rl();
        this.K = rl();
        this.J = rl();
        this.C = []
    }
    kC.prototype.Pa = function() {
        0 != this.C.length && fC(this.F, this.B, this.A, this.H)
    };

    function lC(a) {
        km(mC, a.B);
        var b = a.B.A,
            c = a.B.B,
            d = a.B.D,
            e = a.B.F,
            f = a.B.G - a.B.C,
            g = d - b,
            k = e - c,
            l = f - 0,
            m = El(),
            n = rl(),
            p = rl();
        sl(n, b, c, 0);
        Yl(n, n);
        sl(p, d, e, f);
        Yl(p, p);
        yl(n, nC);
        Rl(m, n, p, nC);
        Nl(m, m);
        Hl(m, 2, oC);
        Hl(m, 1, nC);
        wl(oC, 3.2, oC);
        wl(nC, 1, nC);
        ul(n, oC, n);
        ul(n, nC, n);
        $l(n, n);
        b = nm(a.B);
        b.A = n[0];
        b.B = n[1];
        b.C = n[2];
        b.H = n[0] + g;
        b.I = n[1] + k;
        b.K = n[2] + l;
        lm(a.B, b)
    }
    var oC = rl(),
        nC = rl(),
        gz = El();

    function pC(a, b) {
        for (var c = null, d = 90, e = 0; e < S(b, 19); e++) {
            var f = new Qs(Nc(b, 19, e));
            if (K(f.eb(), 0)) {
                var g = Math.abs(Lb(a, N(f, 0)));
                g < d && (c = f, d = g)
            }
        }
        if (!c) return null;
        a = new wh;
        U(a, c.eb());
        c = Bh(a);
        ze(re(c), Math.max(0, ye(qe(c)) - 3));
        return a
    };

    function qC(a, b, c) {
        this.O = a;
        this.I = b;
        this.F = new zm;
        this.A = c;
        a = new bC;
        b = new Mz;
        Sz(b, [0, 0, 0, .4]);
        b.A = new Float32Array([0, 0, 0, 1, 1, 0, 1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 1, 0]);
        b.translate(0, 0, .01);
        Uz(b, .5);
        c = Oz();
        Sz(c, [0, 0, 0, .1]);
        c.translate(0, 0, .001);
        Uz(c, .92);
        var d = Oz();
        Sz(d, [1, 1, 1, .4]);
        Vl(iz(d), Math.PI / 2);
        Qz(d, [c, b]);
        eC(a, [d, c, b]);
        this.ja = d;
        b = Pz();
        Sz(b, [1, 1, 1, .4]);
        c = [0, 0, 0, .4];
        b.C || (b.C = [1, 1, 1, 1]);
        b.C[0] = c[0];
        b.C[1] = c[1];
        b.C[2] = c[2];
        b.C[3] = c[3];
        Vl(iz(b), Math.PI / 2);
        b.scale(3.23606798, 2, 1);
        this.Y = b;
        a.add(this.Y);
        var e = [0, 0, 0, .1];
        b = [1, 1, 1, .3];
        c = Pz();
        Sz(c, e);
        c.scale(1.9, .15, 1);
        d = Pz();
        Sz(d, e);
        d.translate(0, .5125, 0);
        d.scale(.15, .875, 1);
        var f = Pz();
        Sz(f, e);
        f.translate(0, -.5125, 0);
        f.scale(.15, .875, 1);
        e = Pz();
        Sz(e, b);
        e.scale(2, .25, 1);
        e.translate(0, 0, .001);
        var g = Pz();
        Sz(g, b);
        g.translate(0, .5625, .001);
        g.scale(.25, .875, 1);
        var k = Pz();
        Sz(k, b);
        k.translate(0, -.5625, .001);
        k.scale(.25, .875, 1);
        b = [c, d, f, e, g, k];
        eC(a, b);
        c = new Mz;
        Vl(iz(c), Math.PI / 2);
        Xl(iz(c), Math.PI / 4);
        Qz(c, b);
        this.aa = c;
        this.J = a;
        this.G = !0;
        this.H = this.K = !1;
        this.D =
            new ym;
        this.N = rl();
        this.L = !1;
        this.X = new ym;
        this.U = this.P = 0;
        this.B = this.C = null;
        this.M = !0;
        this.da = rl();
        this.wa = rl();
        this.ga = rl();
        this.ha = rl();
        this.na = rl();
        this.Ga = rl();
        this.xa = new wh;
        this.$ = E(this.I.Tc, this.I)
    }
    var rC = rl(),
        sC = rl(),
        tC = rl();
    qC.prototype.isEnabled = h("G");

    function Nx(a, b) {
        (a.G = b) ? uC(a, a.F.W() / 2, a.F.H / 2): fp(a.O)
    }

    function vC(a) {
        return !!a && .9 < a[2]
    }

    function wC(a) {
        a = a.G && a.K ? a.D.A : null;
        return !!a && .9 >= a[2] && -.9 <= a[2]
    }

    function pz(a) {
        return a.L ? a.N : null
    }
    qC.prototype.Pa = function() {
        if (this.G) {
            var a;
            var b = this.C,
                c = b && b.Ha();
            a = this.aa;
            var d = this.Y,
                e = this.ja;
            d.B.hidden = !0;
            e.B.hidden = !0;
            var f = this.D.origin,
                g = this.D.A,
                k = this.B && this.B.fa();
            if (Zo(b) && k && f && g) {
                this.J && (b = b.Nb(), this.J.G = b);
                var b = this.da,
                    l = this.wa,
                    m = this.na,
                    n = this.ga,
                    p = this.Ga,
                    r = this.ha,
                    c = qe(c.fa());
                dm(we(c), xe(c), ye(c), b);
                k = qe(k);
                am(we(k), xe(k), ye(k), n);
                var k = this.I,
                    c = this.F,
                    u = rl(),
                    t = 1 / c.I,
                    v = c.na,
                    x = n[1],
                    D = n[2];
                v[0] = (n[0] - c.D) * t;
                v[1] = (x - c.F) * t;
                v[2] = (D - c.G) * t;
                v[3] = 1;
                Gl(c.L, Bm(c));
                Ql(c.L, v,
                    v);
                t = 1 / v[3];
                v[0] *= t;
                v[1] *= t;
                v[2] *= t;
                t = u || rl();
                x = v[1];
                D = v[2];
                t[0] = .5 * (v[0] + 1) * c.W();
                t[1] = .5 * (-x + 1) * c.H;
                t[2] = .5 * (D + 1);
                y(k) ? (k.Rc(u[0], u[1], this.X), k = this.X.A, sl(p, k[0], k[1], k[2]), vC(p) || sl(p, 0, 0, 1)) : sl(p, 0, 0, 1);
                Yl(n, n);
                Yl(f, l);
                xC(f, g, l, m);
                xC(f, p, l, p);
                yl(l, r);
                vC(this.G && this.K ? this.D.A : null) ? (vl(n, b, sC), Al(sC, r, rC), Al(m, rC, sC), ul(sC, l, sC), Tz(e, l, sC, r), this.H && Ul(iz(e), .4, .4, .4), e.B.hidden = !1) : wC(this) && (Tz(d, l, m, r), f = Math.min(this.H ? .4 : 1, Math.sqrt(Bl(l, b)) / 5), Ul(iz(d), f, f, f), d.B.hidden = !1);
                a.B.hidden = !1;
                this.M || (d.B.hidden = !0, e.B.hidden = !0, a.B.hidden = !0);
                vl(n, b, sC);
                Al(sC, r, rC);
                Al(p, rC, sC);
                ul(sC, n, sC);
                Tz(a, n, sC, r);
                this.H && Ul(iz(a), .4, .4, .4);
                a = !0
            } else a = !1;
            a && fC(this.J, this.F, this.A, this.$)
        }
    };

    function uC(a, b, c) {
        a.P = b;
        a.U = c;
        var d = a.P,
            e = a.U,
            f = a.I;
        c = !1;
        b = !!f.Rc(d, e, a.D);
        var g = vC(a.D.A),
            k = a.xa;
        y(f) && f.Rd(d, e, k) ? (a.B || (a.B = new wh), U(a.B, k), ze(re(Bh(a.B)), 0), Zo(a.C) && g && Ms(a.B, Ys(a.C.Da())) && (c = !0)) : c = !0;
        d = a.C && a.C.Ha();
        a.H = !!d && 2 == L(d, 6, 1);
        c && a.C && (d ? (e = Ib(-Kb(a.F.O)), a.B = pC(e, d), a.B && (f = Bh(a.B), yC(a, f), c = qe(d.fa()), d = re(f), K(d, 1) || K(d, 2) || (e = e * Math.PI / 180, f = Math.sin(e), g = 180 / Math.PI / 6371010, k = g / Math.cos(xe(c)), d.data[2] = xe(c) + 40 * g * Math.cos(e), d.data[1] = we(c) + 40 * k * f))) : a.B = null);
        a.B && yC(a,
            Bh(a.B));
        a.K = !!a.B && b;
        fp(a.O)
    }

    function yC(a, b) {
        if (Zo(a.C)) {
            var c = a.C.fa(),
                d = !0;
            a.L = !1;
            wC(a) && (c = qe(c), dm(we(c), xe(c), ye(c), rC), c = qe(b), dm(we(c), xe(c), ye(c), sC), vl(sC, rC, tC), d = 60 < xl(tC));
            d ? Jc(b, 1) : a.D.origin && (a.L = !0, tl(a.N, a.D.origin))
        }
    }

    function xC(a, b, c, d) {
        bm(a[0], a[1], a[2], rC, void 0);
        rC[0] = Kb(rC[0]);
        rC[1] = Kb(rC[1]);
        wl(b, cm(rC[1]), d);
        ul(a, d, d);
        Yl(d, d);
        vl(d, c, d);
        yl(d, d)
    }
    qC.prototype.clear = function() {
        this.B = this.C = null
    };

    function zC(a, b) {
        return w.setTimeout(function() {
            try {
                a()
            } catch (c) {
                throw c;
            }
        }, b)
    }

    function AC(a) {
        return w.setInterval(function() {
            try {
                a()
            } catch (b) {
                throw b;
            }
        }, 1E4)
    };

    function BC(a, b) {
        this.G = y(b) ? b : 20;
        this.F = a;
        this.C = this.B = this.A = null;
        var c = this;
        this.D = function() {
            c.B = null;
            c.C = null;
            if (null !== c.A) {
                var a = F();
                a >= c.A - c.G ? (c.A = null, a = c.F, a()) : (c.C = c.A, c.B = zC(c.D, c.A - a))
            }
        }
    }
    BC.prototype.start = function(a) {
        this.A = F() + a;
        if (null !== this.B) {
            if (this.A >= this.C) return;
            w.clearTimeout(this.B)
        }
        this.C = this.A;
        this.B = zC(this.D, a)
    };
    BC.prototype.cancel = function() {
        this.A = null
    };

    function CC(a, b, c) {
        this.G = a;
        this.F = b;
        this.C = c;
        this.B = null;
        var d = this;
        this.D = new BC(function() {
            var a = d.B;
            a && (d.B = null, d.F(a), a.done(d.C))
        })
    }
    CC.prototype.start = function(a) {
        null === this.D.A && (a.la(this.C), this.B && this.B.done(this.C), this.B = a, this.D.start(this.G))
    };
    CC.prototype.A = function() {
        this.D.cancel();
        this.B && (this.B.done(this.C), this.B = null)
    };

    function DC() {}
    DC.prototype.A = aa();

    function EC(a, b) {
        if (a) a: {
            if (a = a.Da()) {
                b = !!b;
                a = Ys(a).ia();
                for (var c = 0; c < S(a, 5); c++)
                    for (var d = uh(a, c), e = 0; e < S(d, 9); e++) {
                        var f = new fg(Nc(d, 9, e)),
                            g = f.ma(),
                            k;
                        (k = !b) || (k = new Zf(f.data[0]), k = K(k, 1) || K(k, 0) || !K(f, 5) ? !1 : !0);
                        if (k && 0 < S(g, 1)) {
                            b = !0;
                            break a
                        }
                    }
            }
            b = !1
        }
        else b = !1;
        return b
    };

    function FC(a, b, c, d, e, f) {
        V.call(this, "AN", wb(arguments))
    }
    H(FC, V);

    function GC() {
        this.M = !1
    }
    GC.prototype.ra = function(a) {
        this.M || (this.M = !0, this.ea(a))
    };
    GC.prototype.ea = aa();

    function HC(a) {
        this.M = !1;
        this.B = a;
        this.A = []
    }
    H(HC, GC);
    q = HC.prototype;
    q.ea = function() {
        for (var a = 0, b = this.A.length; a < b; ++a) this.B.Xc(this.A[a]);
        this.A = []
    };
    q.Xc = function(a) {
        this.B.Xc(a);
        null != a && tb(this.A, a)
    };
    q.Qb = function(a, b, c, d) {
        a = this.B.Qb(a, b, c, d);
        null != a && this.A.push(a);
        return a
    };
    q.wc = function(a, b, c, d, e, f) {
        a = this.B.wc(a, b, c, d, e, f);
        null != a && this.A.push(a);
        return a
    };
    q.tc = function() {
        return this.B.tc()
    };
    var IC = "dragstart drag dragend keypress keydown keyup".split(" ");

    function JC(a) {
        void 0 !== w.history.replaceState ? w.history.replaceState(w.history.state, document.title, a) : w.history.href = a;
        KC()
    }

    function KC() {
        if (w.Mi) {
            var a = w.Mi;
            if (a.Nh && ya(a.Nh) && a.Oh && ya(a.Oh) && a.Re && ya(a.Re)) {
                var b = a.Nh(),
                    c = a.Oh();
                b && a.Re(b);
                for (b = 0; b < c.length; b++) a.Re(c[b])
            }
        }
    };

    function LC(a) {
        this.A = Hy(Fy(), "in");
        (new HC(a)).Qb("popstate", this, this.Wk);
        this.B = !1;
        this.C = Hy(Fy());
        this.C.listen(this.Rk, this);
        this.D = Gy(Fy())
    }
    q = LC.prototype;
    q.listen = function(a, b) {
        this.A.listen(a, b)
    };
    q.set = function(a, b) {
        var c = new lr(w.location.href);
        c.A.set("viewerState", a);
        "lb" === a || "im" === a ? (this.B ? (c = c.toString(), void 0 !== w.history.replaceState ? w.history.pushState(null, document.title, c) : w.history.href = c, KC()) : ("lb" === a && (this.B = !0), c = c.toString(), JC(c)), MC(c)) : "ga" === a && (this.B = !0, c = c.toString(), JC(c), MC(c));
        this.A.set(a, b)
    };
    q.get = function() {
        return this.A.get() || "in"
    };
    q.Wk = function(a) {
        var b = this.get(),
            c;
        c = (c = (new lr(w.location.href)).A.get("viewerState")) ? c : "in";
        "in" !== c && ("lb" === b && "ga" === c ? this.set("ltgl", a) : this.A.set(c, a))
    };
    q.Rk = function() {
        var a = this.D.get(),
            b = this.C.get();
        a && void 0 !== b && (a = ph(a.eb(b).ia()), b = Pe(), a = Yw(a.ib(), b), b = new lr(w.location.href), b.A.set("imagekey", a), a = b.toString(), JC(a), MC(a))
    };
    q.bind = function(a, b, c) {
        By(this.D, a, c);
        By(this.C, b, c)
    };

    function MC(a) {
        try {
            if (a != w.parent.location.href && w.parent && w.parent.google && y(w.parent.google.uvPubSub)) {
                var b = new lr(a);
                w.parent.google.uvPubSub.zn("uup", b.D + "?" + b.A.toString())
            }
        } catch (c) {}
    };

    function $C(a, b) {
        this.C = a;
        this.A = b
    }
    $C.prototype.B = function(a, b) {
        var c;
        c = Ns(a);
        c || (c = qe(a.fa()), c = K(c, 2) && K(c, 1) ? xe(c) + "," + we(c) : "");
        if (!c) return null;
        var d = pl(this.A, c);
        d || (d = this.C.B(a, b), ol(this.A, c, d));
        return d
    };
    $C.prototype.clear = function() {
        this.A.clear()
    };

    function aD(a, b) {
        V.call(this, "CPS", wb(arguments))
    }
    H(aD, V);

    function lD(a, b) {
        V.call(this, "FPS", wb(arguments))
    }
    H(lD, V);

    function mD() {
        V.call(this, "NCS", wb(arguments))
    }
    H(mD, V);

    function nD(a, b) {
        V.call(this, "PPS", wb(arguments))
    }
    H(nD, V);

    function oD(a, b, c, d) {
        V.call(this, "PTI", wb(arguments))
    }
    H(oD, V);

    function pD(a) {
        V.call(this, "SPS", wb(arguments))
    }
    H(pD, V);

    function qD(a) {
        V.call(this, "SPTS", wb(arguments))
    }
    H(qD, V);

    function rD() {
        $p.call(this)
    }
    H(rD, $p);
    rD.prototype.Ha = function() {
        return this.A ? this.A.Ha() : null
    };

    function sD(a, b, c) {
        this.C = a;
        this.J = b;
        this.A = c;
        this.L = this.I = this.H = this.M = this.G = this.F = null;
        this.N = new mD;
        this.K = null;
        this.D = new Rr(this.C)
    }
    sD.prototype.clear = C;
    sD.prototype.B = function(a, b) {
        if (Tr(Ns(a))) return tD(this, a, b);
        var c = Es(a);
        if (c && (4 === L(a, 1, 99) || 3 == L(qh(a.ia()), 0))) {
            var c = uD(this),
                d = new bq;
            c.get(function(b, c) {
                aq(d, b.B(a, c))
            }, b);
            return d
        }
        var e = vD(this, a);
        if (!e) return null;
        if (Fs(a)) return c = new oD(e, a, wD(a)), d = new rD, c.get(function(a) {
            aq(d, a)
        }, b), d;
        if (c) return c = new Gz(e, a), d = new bq, c.get(function(a) {
            aq(d, a)
        }, b), d;
        kb("Unable to getRenderable for Photo: " + Pc(a));
        return null
    };

    function tD(a, b, c) {
        var d = Ns(b),
            e = new Vr(a.C, d),
            d = new it(d);
        a = new Gz(new tt(d, e, a.J), b);
        var f = new bq;
        a.get(function(a) {
            aq(f, a)
        }, c);
        return f
    }

    function vD(a, b) {
        var c;
        c = O(a.A, 75) ? xD(a) : a.N;
        b = yD(a, b);
        return c && b ? new tt(c, b, a.J) : null
    }

    function yD(a, b) {
        var c = qh(b.ia());
        if (K(c, 0) && K(c, 1)) switch (L(c, 0)) {
            case 1:
                return 3 == L(c, 1) && K(c.eb(), 0) && !Gs(b) ? new qD(zD(a)) : AD(a);
            case 2:
                return 3 == L(c, 1) ? (a.F || (a.F = new aD(a.D, cc(a.A.data, 13))), a = new qD(a.F)) : a = BD(a), a;
            case 8:
            case 3:
                return new qD(zD(a));
            case 4:
                return new qD(CD(a))
        }
        b = zh(b);
        return 1 === b || 2 === b || 11 === b || 13 === b || 5 === b || 4 === b ? BD(a) : 3 === b ? new qD(BD(a)) : 10 === b ? new qD(CD(a)) : 12 === b || 15 === b ? new qD(zD(a)) : 27 === b ? AD(a) : null
    }

    function wD(a) {
        var b = qh(a.ia());
        if (K(b, 0) && K(b, 1)) switch (L(b, 0)) {
            case 2:
                return 3 == L(b, 1) ? [0, 0, 0, 0, 85, 320, 512, 768, 1024] : [];
            case 1:
            case 8:
            case 3:
                return [512, 1024, 1536];
            case 4:
                return [512, 1024, 1280]
        }
        a = zh(a);
        return 10 === a ? [512, 1024, 1280] : 12 === a || 15 === a ? [512, 1024, 1536] : 3 === a ? [0, 0, 0, 0, 85, 320, 512, 768, 1024] : []
    }

    function CD(a) {
        a.M || (a.M = new nD(a.D, O(a.A, 64)));
        return a.M
    }

    function zD(a) {
        a.I || (a.I = new lD(a.D, cc(a.A.data, 73)));
        return a.I
    }

    function BD(a) {
        a.G || (a.G = new ot(a.D, cc(a.A.data, 13)));
        return a.G
    }

    function uD(a) {
        if (!a.H) {
            var b = null;
            O(a.A, 75) && (b = xD(a));
            a.H = new qt(a.C, a.J, cc(a.A.data, 73), b)
        }
        return a.H
    }

    function AD(a) {
        a.L || (a.L = new pD(a.C));
        return a.L
    }

    function xD(a) {
        if (!a.K) {
            var b = new od;
            b.data[0] = O(new Ee(a.A.data[16]), 0);
            b.data[1] = O(new Ee(a.A.data[16]), 1);
            a.K = new Fw(O(a.A, 75), O(a.A, 85), Ic(a.A, 86), a.C, K(a.A, 87) ? O(a.A, 87) : "maps_sv.tactile", b, a.A.A ? new vd(a.A.data[89]) : null)
        }
        return a.K
    };

    function DD(a, b, c, d, e, f) {
        il.call(this);
        this.D = b;
        this.N = c;
        this.C = d;
        this.I = e;
        this.F = !1;
        this.J = new le;
        this.A = new Pn(0, 0, 0, 0);
        this.L = 0;
        this.H = !1;
        this.B = null;
        this.G = !1;
        var g = this;
        this.D.C(function(b, c) {
            g.G && (b.$b(c), g.G = !1);
            Vk(b, "ViewportReady", function() {
                g.D == ED(g) && (g.F = !0, g.dispatchEvent(new Ek("ViewportReady", g)))
            });
            g.H && (fp(a), g.H = !1)
        }, f);
        this.C && this.C.C(function(b, c) {
            g.G && (b.$b(c), g.G = !1);
            Vk(b, "ViewportReady", function() {
                g.C == ED(g) && (g.F = !0, g.dispatchEvent(new Ek("ViewportReady", g)))
            });
            g.H && (fp(a),
                g.H = !1)
        }, f)
    }
    H(DD, il);
    q = DD.prototype;
    q.xc = function(a, b) {
        U(this.J, a);
        var c = this;
        ED(this).get(function(a, b) {
            a.xc(c.J, b)
        }, b)
    };
    q.hc = function(a, b, c) {
        var d = !!qb(a, function(a) {
            var b;
            if (b = !!a.Da()) b = (a = Ys(a.Da())) ? K(qh(a.ia()), 0) ? 1 == L(qh(a.ia()), 0) : 7 == L(a, 1, 99) : !1;
            return b
        });
        FD(this, d && this.C ? this.C : this.D);
        var e = this;
        ED(this).get(function(b, d) {
            b.xc(e.J, d);
            b.Zc(e.L, d);
            b.yc(e.A.top, e.A.right, e.A.bottom, e.A.left, d);
            b.hc(a, d, c)
        }, b)
    };
    q.clear = function() {
        this.D.B() && this.D.A().clear();
        this.C && this.C.B() && this.C.A().clear()
    };
    q.Pa = function() {
        this.F = !1;
        var a = ED(this);
        a.B() ? a.A().Pa() : this.H = !0
    };
    q.$b = function(a) {
        this.F = !1;
        var b = ED(this);
        b.B() ? b.A().$b(a) : this.G = !0
    };
    q.yc = function(a, b, c, d, e) {
        this.A.top = a;
        this.A.right = b;
        this.A.bottom = c;
        this.A.left = d;
        var f = this;
        ED(this).get(function(a, b) {
            a.yc(f.A.top, f.A.right, f.A.bottom, f.A.left, b);
            a.Pa()
        }, e)
    };
    q.Yc = h("F");

    function ED(a) {
        if (a.B) return a.B;
        FD(a, a.D);
        return a.D
    }

    function FD(a, b) {
        a.B != b && (a.B && a.B.B() && a.B.A().clear(), a.B = b, b = a.B == a.D, a.N && ao(a.N, b), a.I && (ao(a.I, !b), b = b ? "0" : "100%", Wn(a.I, b, b)))
    }
    q.Zc = function(a, b) {
        this.L = a;
        var c = this;
        ED(this).get(function(a, b) {
            a.Zc(c.L, b)
        }, b)
    };

    function GD(a, b, c, d, e, f) {
        DD.call(this, a, b, c, d, e, f)
    }
    H(GD, DD);
    GD.prototype.Rd = function(a, b, c) {
        var d = ED(this);
        return d.B() ? d.A().Rd(a, b, c) : null
    };
    GD.prototype.Rc = function(a, b, c) {
        var d = ED(this);
        return d.B() ? d.A().Rc(a, b, c) : null
    };
    GD.prototype.Tc = function(a, b) {
        var c = ED(this);
        return c.B() ? c.A().Tc(a, b) : null
    };
    GD.prototype.Yd = function(a) {
        var b = ED(this);
        b.B() ? b.A().Yd(a) : (a[0] = 1, a[1] = 179)
    };

    function HD(a, b, c) {
        V.call(this, "WPNR", wb(arguments))
    }
    H(HD, V);

    function ID(a, b, c, d, e, f, g, k) {
        this.G = a;
        e = new sD(e, this.G.A, d);
        this.da = new $C(e, new nl(30));
        this.ha = 2 === L(d, 20, 1);
        this.Y = !0;
        var l;
        e = b.B;
        if (c.yb && this.ha) {
            l = this.G;
            var m = b.canvas.A;
            b = b.B;
            var n = new Cj(l, c.yb, Rj("CANVAS"), void 0),
                p = new jw(l, b);
            l = new GD(l, n, m, p, b, k)
        } else c.Sa && 1 === L(d, 20, 1) ? (l = this.G, m = b.canvas.A, b = b.B, n = new HD(l, c.Sa, void 0), p = new jw(l, b), l = new GD(l, n, m, p, b, k)) : e && (b = this.G, l = new jw(b, e), l = new GD(b, l, e, null, null, k));
        this.D = l;
        this.X = this.aa = !1;
        c.A = e || c.A;
        this.A = new qC(a, this.D, c);
        this.O =
            new kC(this.D, c);
        this.N = !1;
        this.P = null;
        this.H = 0;
        this.ga = Ic(d, 88) && !Ic(d, 92);
        this.B = new FC(this.D, c, a, f, g, this.ga);
        this.C = null;
        this.F = !1;
        this.J = void 0;
        this.L = null;
        this.K = void 0;
        this.I = this.M = null;
        this.xa = C;
        this.na = Ic(d, 88);
        this.U = null;
        this.$ = Gy(Fy());
        this.$.listen(this.wa, this)
    }

    function fz(a, b, c, d) {
        a.F && a.B.B() && a.B.A().K(d);
        uC(a.A, b, c);
        var e = a.A.B;
        e && (e = Ds(e));
        !a.H || !a.P || e && Oc(a.P, e) || (Oy(a.H), a.H = 0);
        !a.H && e && (a.P = e, a.H = Ny(function() {
            (e = a.A.B) && Oc(a.P, e) && Kx(a, e, d).Rb(d);
            a.H = 0
        }, 250, d, "prd-update-cursor"))
    }

    function Lx(a, b, c, d) {
        a.C = b;
        a.C.listen("TileReady", function(b) {
            a.xa(b, c)
        });
        a.J = d;
        a.A.clear();
        a.A.C = b;
        Ez(a, c)
    }

    function Ez(a, b) {
        a.F = a.na && EC(a.C, a.ga);
        a.F && a.B.get(function(b, d) {
            b.H(a.C);
            b.A(a.U);
            d.ua("arp");
            fp(a.G)
        }, b)
    }

    function oz(a, b, c) {
        return a.F && a.B.B() ? a.B.A().D(b, c) : !1
    }

    function JD(a, b, c) {
        if (a.F && a.B.B()) {
            var d = a.B.A();
            if (a = a.C ? a.C.fa() : null) return d.C(b, c, a)
        }
        return null
    }

    function vz(a, b) {
        return (a = a.C.Ha()) && S(a, 19) ? pC(b, a) : null
    }
    ID.prototype.Pa = function() {
        this.D.Pa();
        this.A.Pa();
        this.N && this.O.Pa();
        KD(this)
    };

    function Wy(a, b) {
        a.U = b;
        a.F && a.B.B() && a.B.A().A(b)
    }

    function KD(a) {
        if (a.F && a.B.B()) {
            var b = a.B.A();
            b.F();
            b.A(a.U)
        }
    }

    function Az(a, b) {
        a.X = !1;
        LD(a);
        a.M = new CC(15E3, function() {
            a.C = null;
            a.J = void 0;
            a.L = null;
            a.K = void 0;
            a.I && a.I();
            a.da.clear();
            a.D.clear();
            a.A.clear()
        }, "clear-pano-render-cache");
        a.M.start(b);
        a.H && (Oy(a.H), a.H = 0)
    }

    function Mx(a, b) {
        var c = [],
            d = [];
        if (a.C) {
            a.J || (a.J = new Vo, a.J.data[0] = 1);
            var e = a.C,
                f = a.J;
            e && (c.push(e), f.data[4] = a.Y ? 1 : 0, d.push(f))
        }
        a.L && (a.K || (a.K = new Vo, a.K.data[0] = 1), e = a.L, f = a.K, e && (c.push(e), f.data[4] = a.Y ? 1 : 0, d.push(f)));
        a.D.hc(c, b, d);
        fp(a.G)
    }

    function Jx(a, b, c) {
        var d = Kx(a, b, c);
        if (d) {
            Lx(a, d, c, void 0);
            a.L = null;
            a.K = void 0;
            Nx(a.A, !0);
            Mx(a, c);
            var e = E(function() {
                    var a = this.O,
                        b = this.C;
                    if (b && !(a.D && b.ia() && Oc(ph(b.ia()), ph(a.D.ia())))) {
                        var c = b.Ha();
                        if (c) {
                            a.D = b;
                            a.C = [];
                            a.F.A = [];
                            var d = qe(c.fa()),
                                b = gz,
                                e = a.I,
                                f = a.K;
                            dm(we(d), xe(d), 0, e);
                            dm(we(d), xe(d) + 1E-5, 0, f);
                            d = a.J;
                            tl(d, e);
                            yl(d, d);
                            Rl(b, e, f, d);
                            Nl(b, b);
                            b = S(c, 19);
                            for (e = 0; e < b; e++)
                                if (f = new Qs(Nc(c, 19, e)), K(f.eb(), 0)) {
                                    var r = [0, 0, 0, 1, .6, 0, 1, 0, 0, 0, -.6, 0, -1, 0, 0, -1, .6, 0],
                                        d = new Mz;
                                    Sz(d, [.9, .9, .9, 1]);
                                    var u = new Float32Array(r);
                                    d.A = u;
                                    Vl(iz(d), Math.PI / 2);
                                    Uz(d, .25);
                                    u = new Mz;
                                    Sz(u, [0, 0, 0, .6]);
                                    r = new Float32Array(r);
                                    u.A = r;
                                    Vl(iz(u), Math.PI / 2);
                                    Uz(u, .25);
                                    u.translate(0, 0, .15);
                                    d = [u, d];
                                    u = new Mz;
                                    Qz(u, d);
                                    eC(a.F, d);
                                    var r = a,
                                        t = N(f, 0);
                                    Fl(iz(u), gz);
                                    if (K(r.D.fa(), 1)) {
                                        var v = N(se(r.D.fa()), 1),
                                            r = Ae(se(r.D.fa()));
                                        Wl(iz(u), Jb(-r));
                                        Vl(iz(u), Jb(v - 90));
                                        Wl(iz(u), Jb(r))
                                    }
                                    Wl(iz(u), Jb(-t));
                                    u.translate(0, 0, -.75);
                                    for (r = 0; r < d.length; r++) a.C.push({
                                        shape: d[r],
                                        target: f.eb()
                                    })
                                }
                        }
                    }
                }, a),
                f = E(aa(), a);
            MD(a, b, function(a, b) {
                e();
                f();
                a && b.ua("vrp")
            }, c)
        }
    }
    var mC = new le;

    function zz(a) {
        return a.C ? (a = a.C.Da()) ? Zs(a) : null : null
    }

    function Kx(a, b, c) {
        if (!b) return null;
        (a = a.da.B(b, c)) && a.Rb(c);
        return a
    }

    function Bz(a, b, c) {
        U(mC, b);
        Qy(mC);
        km(mC, a.A.F);
        lC(a.O);
        a.F && a.B.get(function(a) {
            a.G(mC)
        }, c);
        a.D.xc(mC, c);
        a.aa = !0;
        fp(a.G)
    }

    function Ox(a, b, c, d) {
        function e() {
            d && !g && (g = !0, d(!f, c))
        }
        Kx(a, Ys(b), c);
        var f = !1,
            g = !1;
        e();
        return new Dr(function() {
            f = !0;
            e()
        })
    }

    function rz(a, b, c, d) {
        function e(a, b, d, e) {
            e && U(Ah(f), e);
            c(a, b, d, f)
        }
        var f = new wh;
        U(f, b);
        return ND(a, f, e, e, d)
    }

    function MD(a, b, c, d) {
        ND(a, b, function(b, d, g) {
            if (a.D.Yc()) c(b, g);
            else {
                g.la("pano-wait-for-content");
                var e = bl(a.D, "ViewportReady", function() {
                    a.I = null;
                    c(b, g);
                    g.done("pano-wait-for-content")
                });
                a.I && a.I();
                a.I = function() {
                    dl(e);
                    a.I = null;
                    g.done("pano-wait-for-content")
                }
            }
        }, function(a, b, d) {
            c(a, d)
        }, d)
    }

    function ND(a, b, c, d, e) {
        if (!b) return d(!1, null, e, null), new DC;
        var f = Kx(a, b, e);
        if (!f) return d(!1, null, e, null), new DC;
        var g = !1;
        f.dc(e.sa(function(a) {
            g || 0 === a ? d(!1, null, e, null) : 4 === a ? d(!1, null, e, null) : c(!0, f.fa(), e, f.ia())
        }, "pano-wait-for-content"));
        return new Dr(function() {
            g = !0
        })
    }
    ID.prototype.Ib = C;
    ID.prototype.ja = Ui;

    function xz(a, b, c) {
        return b && (a = Kx(a, b, c), Zo(a) && K(a.Ha(), 23)) ? N(a.Ha(), 23) : null
    }
    ID.prototype.wb = function() {
        this.X = !0;
        LD(this)
    };

    function LD(a) {
        a.M && a.M.A();
        a.M = null
    }
    ID.prototype.wa = function() {
        var a = this.$.get();
        a && (a = "im" == a, a !== this.N && fp(this.G), this.N = a)
    };

    function OD(a, b, c, d, e, f, g, k) {
        d.getContext(function(b, m) {
            a(new ID(c, d, b, e, f, g, k, m))
        }, b)
    };

    function PD(a, b) {
        this.A = a;
        this.D = ua(b) ? b : [b];
        this.B = [];
        this.C = []
    }
    PD.prototype.Xc = function(a) {
        this.A.Xc(a);
        if (null != a && (tb(this.C, a), 0 == this.C.length && 0 < this.B.length)) {
            for (a = 0; a < this.B.length; a++) {
                var b = this.A.tc(),
                    c = this.B[a];
                QD(c);
                for (var d = !1, e = 0; e < b.A.length; ++e)
                    if (b.A[e] === c) {
                        b.A.splice(e, 1);
                        d = !0;
                        break
                    }
                if (!d)
                    for (e = 0; e < b.B.length; ++e)
                        if (b.B[e] === c) {
                            b.B.splice(e, 1);
                            break
                        }
                RD(b)
            }
            this.B.length = 0
        }
    };
    PD.prototype.Qb = function(a, b, c, d) {
        return this.A.Qb(a, b, c, d)
    };
    PD.prototype.wc = function(a, b, c, d, e, f) {
        a = this.A.wc(a, b, c, d, e, f);
        if (null != a) {
            if (0 == this.C.length)
                for (b = 0; b < this.D.length; b++) {
                    c = this.A.tc();
                    e = this.D[b];
                    d = new SD(e);
                    b: {
                        for (f = 0; f < c.A.length; f++)
                            if (TD(c.A[f].A, e)) {
                                e = !0;
                                break b
                            }
                        e = !1
                    }
                    e ? c.B.push(d) : (UD(c, d), c.A.push(d), RD(c));
                    c = d;
                    this.B.push(c)
                }
            this.C.push(a)
        }
        return a
    };
    PD.prototype.tc = function() {
        return this.A.tc()
    };
    var VD = Fy();
    var WD = Fy();
    var XD = Fy();

    function YD(a, b) {
        this.X = a;
        this.Y = b || 0;
        this.F = []
    }
    YD.prototype.O = function(a) {
        a.la("maps-consumer-kvo-view-redraw-later");
        this.F.push(a);
        this.J || (this.J = !0, a = this.Y, 0 == a ? this.P() : zC(E(this.P, this), a))
    };
    YD.prototype.start = function() {
        this.H();
        return rj
    };
    YD.prototype.H = function() {
        this.J = !1;
        var a = this.F;
        this.F = [];
        this.N(a[0]);
        for (var b = 0; b < a.length; ++b) a[b].done("maps-consumer-kvo-view-redraw-later")
    };
    YD.prototype.P = function() {
        var a = this.X;
        a.G.push(this);
        ZD(a.B)
    };

    function $D(a) {
        var b = Gy(VD, !0);
        b.listen(a.O, a);
        return b
    }

    function aE(a) {
        var b = Gy(XD, void 0);
        b.listen(a.O, a);
        return b
    };

    function bE(a) {
        this.A = a || Rj("CANVAS");
        this.B = [];
        this.C = 1
    }
    bE.prototype.addEventListener = function(a, b) {
        this.B.push({
            type: a,
            listener: b
        });
        this.A.addEventListener(a, b, !1)
    };
    bE.prototype.removeEventListener = function(a, b) {
        for (var c = 0; c < this.B.length; c++)
            if (b === this.B[c].listener && a === this.B[c].type) {
                this.B.splice(c, 1);
                break
            }
        this.A.removeEventListener(a, b, !1)
    };
    bE.prototype.dispatchEvent = function(a) {
        for (var b = !1, c = 0; c < this.B.length; c++)
            if (a.type == this.B[c].type) var d = this.B[c].listener,
                b = "function" === typeof d ? b | d(a) : b | d.handleEvent(a);
        return b
    };

    function cE(a, b, c, d, e, f) {
        YD.call(this, d);
        this.M = a;
        this.A = b;
        this.B = aE(this);
        this.C = $D(this);
        this.D = c;
        this.U = e;
        this.K = f;
        this.I = !1;
        this.G = new oe;
        a = this.D.A;
        0 == a.width && (a.width = 1);
        0 == a.height && (a.height = 1)
    }
    H(cE, YD);
    cE.prototype.L = function(a, b) {
        this.G = a;
        ZB(this.D, this.U, w.devicePixelRatio || 1, this.G.W(), Ce(this.G));
        this.K && (.75 > this.D.C && (this.I = !0), 1 <= this.D.C && (this.I = !1), this.K(this.I, b))
    };
    cE.prototype.N = function(a) {
        var b = this.B.get() || "";
        this.A.data[0] = b;
        b = !!this.C.get();
        this.A.data[2] = b;
        this.M.fill(this.A);
        dE(this.M, a.sa(C, "scene.template-render"))
    };

    function eE(a) {
        var b = {
                x: a.x,
                y: a.y,
                me: 0,
                type: a.type,
                altKey: a.altKey,
                ctrlKey: a.ctrlKey,
                shiftKey: a.shiftKey,
                metaKey: a.metaKey,
                button: a.button
            },
            c = null;
        a.touches && 0 < a.touches.length ? c = a.touches : a.changedTouches && 0 < a.changedTouches.length && (c = a.changedTouches);
        if (c) {
            a = c[0];
            var d = c[c.length - 1],
                e = a.target,
                c = a.clientX - e.clientLeft;
            a = a.clientY - e.clientTop;
            var f = d.clientX - e.clientLeft,
                d = d.clientY - e.clientTop,
                e = f - c,
                g = d - a;
            b.x = (c + f) / 2;
            b.y = (a + d) / 2;
            b.me = Math.sqrt(e * e + g * g)
        }
        return b
    };

    function fE(a) {
        Ek.call(this, "RenderComplete", a)
    }
    H(fE, Ek);

    function gE() {
        this.A = this.B = !1
    };

    function hE(a) {
        return (a = a.exec($a)) ? a[1] : ""
    }
    var iE = function() {
        if (Fd) return hE(/Firefox\/([0-9.]+)/);
        if (Rc || Sc || Qc) return dd;
        if (Jd) return hE(/Chrome\/([0-9.]+)/);
        if (Kd && !(Zb() || Yb("iPad") || Yb("iPod"))) return hE(/Version\/([0-9.]+)/);
        if (Gd || Hd) {
            var a = /Version\/(\S+).*Mobile\/(\S+)/.exec($a);
            if (a) return a[1] + "." + a[2]
        } else if (Id) return (a = hE(/Android\s+([0-9.]+)/)) ? a : hE(/Version\/([0-9.]+)/);
        return ""
    }();
    var jE = ["webgl", "experimental-webgl", "moz-webgl"],
        kE = 0;

    function lE(a, b) {
        var c = b || new gE;
        b = b || new gE;
        b = {
            alpha: !0,
            stencil: !0,
            preserveDrawingBuffer: b.B,
            failIfMajorPerformanceCaveat: !b.A && !0
        };
        Tc && !ed(25) && (b.preserveDrawingBuffer = !0);
        a: {
            for (var d = null, e = jE.length, f = 0; f < e; ++f) {
                try {
                    d = a.getContext(jE[f], b)
                } catch (g) {}
                if (d) {
                    b = d;
                    break a
                }
            }
            b = null
        }
        if (!b) return kE = 1, null;
        b.getExtension("WEBGL_debug_renderer_info") ? (d = b.getParameter(37446), mE = nE(d)) : mE = null;
        if (b.drawingBufferWidth != a.width || b.drawingBufferHeight != a.height) return kE = 2, null;
        if (4 > b.getParameter(35660)) return kE =
            3, null;
        a = b.getParameter(3379);
        if (y(void 0) && void 0 > a) return kE = 6, null;
        if (23 > b.getShaderPrecisionFormat(35632, 36338).precision) return kE = 4, null;
        a = mE;
        return Rc && !a ? (kE = 8, null) : !c.A && a && ("Intel Q45" == a && (Rc || Fd) || -1 != oE.indexOf(a)) ? (kE = 5, null) : b
    }
    var mE = void 0;

    function nE(a) {
        if (void 0 === a) return null;
        a = a.toLowerCase();
        var b = a.match(/angle \((.*)\)/);
        b && (a = b[1], a = a.replace(/\s*direct3d.*$/, ""));
        a = a.replace(/\s*\([^\)]*wddm[^\)]*\)/, "");
        var c = a;
        0 > c.indexOf("intel") ? b = null : (b = ["Intel"], 0 <= c.indexOf("mobile") && b.push("Mobile"), (0 <= c.indexOf("gma") || 0 <= c.indexOf("graphics media accelerator")) && b.push("GMA"), 0 <= c.indexOf("haswell") ? b.push("Haswell") : 0 <= c.indexOf("ivy") ? b.push("HD 4000") : 0 <= c.indexOf("sandy") ? b.push("HD 3000") : 0 <= c.indexOf("ironlake") ? b.push("HD") :
            (0 <= c.indexOf("hd") && b.push("HD"), (c = c.match(pE)) && b.push(c[1].toUpperCase())), b = b.join(" "));
        if (b) return b;
        b = a;
        if (0 > b.indexOf("nvidia") && 0 > b.indexOf("quadro") && 0 > b.indexOf("geforce") && 0 > b.indexOf("nvs")) b = null;
        else {
            c = ["nVidia"];
            0 <= b.indexOf("geforce") && c.push("geForce");
            0 <= b.indexOf("quadro") && c.push("Quadro");
            0 <= b.indexOf("nvs") && c.push("NVS");
            b.match(/\bion\b/) && c.push("ION");
            b.match(/gtx\b/) ? c.push("GTX") : b.match(/gts\b/) ? c.push("GTS") : b.match(/gt\b/) ? c.push("GT") : b.match(/gs\b/) ? c.push("GS") :
                b.match(/ge\b/) ? c.push("GE") : b.match(/fx\b/) && c.push("FX");
            var d = b.match(pE);
            d && c.push(d[1].toUpperCase().replace("GS", ""));
            0 <= b.indexOf("titan") ? c.push("TITAN") : 0 <= b.indexOf("ti") && c.push("Ti");
            b = c.join(" ")
        }
        if (b) return b;
        c = a;
        0 > c.indexOf("amd") && 0 > c.indexOf("ati") && 0 > c.indexOf("radeon") && 0 > c.indexOf("firegl") && 0 > c.indexOf("firepro") ? b = null : (b = ["AMD"], 0 <= c.indexOf("mobil") && b.push("Mobility"), d = c.indexOf("radeon"), 0 <= d && b.push("Radeon"), 0 <= c.indexOf("firepro") ? b.push("FirePro") : 0 <= c.indexOf("firegl") &&
            b.push("FireGL"), 0 <= c.indexOf("hd") && b.push("HD"), (c = (0 <= d ? c.substring(d) : c).match(pE)) && b.push(c[1].toUpperCase().replace("HD", "")), b = b.join(" "));
        return b ? b : a.substring(0, 100)
    }
    var pE = /([a-z0-9]*\d+[a-z0-9]*)/,
        oE = "microsoft basic render driver;vmware svga 3d;Intel 965GM;Intel B43;Intel G41;Intel G45;Intel G965;Intel GMA 3600;Intel Mobile 4;Intel Mobile 45;Intel Mobile 965".split(";");

    function qE(a, b) {
        this.M = !1;
        var c = this;
        this.D = a;
        this.A = b;
        this.F = new wq(this);
        yq(this.F, a, "RenderComplete", this.L, !1, this);
        this.G = this.C = this.B = null;
        rE++;
        this.I = this.H = !1;
        this.J = new sE(b, this);
        b.Sa && this.F.listen(b.Sa, "webglcontextrestored", function() {
            tE(c, "contextrestored");
            fp(a)
        });
        this.K = AC(function() {
            tE(c, "timer")
        })
    }
    H(qE, GC);
    var rE = 1;

    function uE(a) {
        if (a.I || !a.B) a = !1;
        else if (a = a.B, a.C)
            if (a.X) {
                var b = a.C.sb();
                a = a.D.Yc() && !a.aa && (2 == b || 3 == b || 4 == b)
            } else a = !1;
        else a = !0;
        return a
    }

    function vE(a, b) {
        uE(a) ? b() : a.C ? a.C.push(b) : a.C = [b]
    }
    qE.prototype.L = function() {
        var a = uE(this),
            b = this.B ? this.B.ja() : !1;
        this.H != a && (this.H = a) && (this.D.A.xa(0), F());
        if (this.C && a) {
            a = this.C;
            this.C = null;
            for (var c = 0; c < a.length; c++) a[c]()
        }
        if (this.G && b)
            for (b = this.G, this.G = null, a = 0; a < b.length; a++) b[a]()
    };

    function tE(a, b) {
        var c = a.J;
        wE(c, b);
        if (!c.C) {
            c.C = !0;
            var d = a.D.A;
            vE(a, function() {
                jq(d, c, kq(0, !1))
            })
        }
    }
    qE.prototype.ea = function() {
        this.F.ra();
        w.clearInterval(this.K)
    };

    function sE(a, b) {
        this.A = a;
        this.C = !1;
        this.B = {};
        this.D = "";
        this.F = b.D.A
    }
    sE.prototype.start = function() {
        if (0 < xE(this.F)) {
            var a = this.F;
            a.U.push({
                fk: this,
                Qa: 0
            });
            yE(a.B);
            return rj
        }
        this.C = !1;
        this.B = {};
        this.D = "";
        return rj
    };

    function wE(a, b) {
        a.B[b] || (a.D += b + ";", a.B[b] = !0)
    };

    function zE(a, b) {
        this.A = a;
        this.D = b;
        this.C = this.B = null
    }

    function AE(a, b, c, d) {
        a.C = b;
        d = c.sa(E(a.H, a, c, d), "transition-default-", "df0", "df1");
        var e = a.A;
        e.xa = !0;
        e.D.A();
        e.F.A++;
        BE(b, E(a.F, a, c, d))
    }
    zE.prototype.F = function(a, b, c) {
        if (c) {
            var d = E(function() {
                BE(this.C, E(this.F, this, a, b))
            }, this);
            CE(new DE(this.A, c, this.D, d, a))
        } else b()
    };
    zE.prototype.H = function(a, b) {
        this.C = null;
        var c = this.A,
            d = c.S.get(),
            e = c.G.get();
        U(e, d);
        EE(c.F);
        c.xa = !1;
        b(a)
    };
    zE.prototype.G = function(a, b, c) {
        this.B = null;
        var d = this.A,
            e = d.S.get(),
            f = d.G.get();
        U(f, e);
        d.D.start(b);
        d.K.x = -1;
        d.K.y = -1;
        EE(d.F, d.K);
        if (0 <= d.K.x && 0 <= d.K.y && d.B.I) {
            var e = d.B.I,
                f = d.K.x,
                d = d.K.y,
                g = b.event();
            g && ez(e, g);
            fz(e.A, f, d, b)
        }
        a(b, c)
    };

    function DE(a, b, c, d, e) {
        this.F = a;
        this.C = b;
        this.G = this.A = !1;
        this.D = c;
        this.H = d;
        this.oa = e
    }
    DE.prototype.B = function(a) {
        var b = this.C.G(a, this.oa);
        if (b) {
            var c = this.F,
                d = this.oa,
                e = c.S.get();
            U(e, b);
            FE(c, e);
            Dy(c.S, d)
        }(b = this.C.H(a)) && this.F.Ib(b, this.oa);
        1 === a && this.finish(!1, this.oa)
    };

    function CE(a) {
        a.A || (a.A = !0, nz(a.D, a, a.C.D()))
    }
    DE.prototype.finish = function(a) {
        if (this.A) {
            this.A = !1;
            this.G = a;
            a: {
                a = this.D;
                var b;
                for (b = 0; b < a.A.length; b++)
                    if (a.A[b].A == this) {
                        a.A[b] = a.A[a.A.length - 1];
                        a.A.pop();
                        break a
                    }
                for (b = 0; b < a.B.length; b++) {
                    var c = a.B[b];
                    if (c.A == this) {
                        a.C.push(c);
                        break
                    }
                }
            }
            this.H(this.oa, this)
        }
    };
    DE.prototype.cancel = function(a) {
        this.finish(!0, a)
    };

    function GE(a, b) {
        this.x = a;
        this.y = b
    }
    H(GE, Dj);
    GE.prototype.scale = Dj.prototype.scale;
    GE.prototype.add = function(a) {
        this.x += a.x;
        this.y += a.y;
        return this
    };
    GE.prototype.rotate = function(a) {
        var b = Math.cos(a);
        a = Math.sin(a);
        var c = this.y * b + this.x * a;
        this.x = this.x * b - this.y * a;
        this.y = c;
        return this
    };
    var HE = Fy();
    var IE = Fy();
    var JE = Fy();
    var KE = Fy();
    var LE = Fy();
    var ME = null;

    function NE(a, b) {
        qo && (ME || (ME = [], Vk(qo, "beforedone", function(a) {
            for (var b = ME, c = 0, f = b.length; c < f; c++) {
                var g = b[c].key,
                    k = b[c].value;
                (k = ya(k) ? k(a.oa) : k) && !a.oa.Nc[g] && vo(a.oa, g, k)
            }
        })), ME.push({
            key: a,
            value: b
        }))
    };

    function OE() {
        this.S = Hy(Iy);
        this.L = Gy(Iy);
        this.B = Hy(Ky);
        this.K = Gy(Ly);
        this.A = Hy(Jy);
        this.G = this;
        this.xa = this.I = this.P = this.na = null
    }
    q = OE.prototype;
    q.Fg = C;
    q.wb = C;
    q.nf = function(a, b) {
        By(a, this.A, b)
    };
    q.Hc = ca(!1);
    q.Gg = ca(!1);
    q.Wf = C;
    q.Te = ca(!1);
    q.Tg = ca(!1);
    q.Rg = C;
    q.Ug = C;
    q.Ue = C;
    q.Ve = C;
    q.Sg = C;
    q.Se = C;
    q.Qg = C;
    q.Hg = function(a, b) {
        var c = this.S.get();
        c && (U(ve(c), a), Dy(this.S, b))
    };
    q.Ge = ca("n");
    q.Gc = C;
    q.Ed = C;
    q.Ec = ca(!1);

    function PE(a, b, c, d, e, f, g, k, l, m, n, p, r, u) {
        var t = this;
        this.Xd = u;
        this.S = Hy(Iy);
        this.S.listen(this.rk, this);
        this.Zb = Hy(LE, this);
        this.Pb = Hy(KE, this);
        this.C = Hy(Ky);
        NE("sc", function() {
            return t.C.get() ? "" + Xs(t.C.get()) : ""
        });
        this.Um = Gy(Ly, this);
        this.X = this.aa = this.xa = !1;
        this.A = f;
        this.L = new qE(e, f);
        this.F = g;
        this.Pe = Gy(JE, this);
        this.Fb = Hy(HE, this);
        this.width = Gy(WD);
        this.height = Gy(WD);
        this.ha = Gy(IE);
        this.ha.listen(this.Eg, this);
        this.G = Hy(Iy);
        this.Ta = new le;
        this.Ua = new Vs;
        this.ja = null;
        this.U = 0;
        this.Db = Gy(VD);
        this.Ga = Gy(VD);
        this.B = this.H = new OE;
        By(this.B.S, this.S, n);
        NE("drv", function() {
            return t.B.Ge()
        });
        this.Nm = r;
        this.N = null;
        this.I = Hy(XD);
        this.Xe = Hy(VD, !0);
        this.ga = Hy(Jy);
        this.wa = new zE(this, k);
        this.Ma = b;
        this.Qm = d;
        this.J = !1;
        this.P = this.O = 0;
        this.da = !1;
        this.na = this.$ = null;
        this.D = new CC(100, function(a) {
            var b = t.S.get(),
                c = t.G.get();
            t.B === t.H ? (t.D.A(), t.D.start(a)) : (U(c, b), Dy(t.G, a))
        }, "stableCameraUpdaterFuse");
        this.K = new GE(-1, -1);
        this.Ye = m;
        this.Y = this.Wa = null;
        this.Oe = n
    }
    H(PE, GC);
    var QE = 1 / 6;
    q = PE.prototype;
    q.ea = function(a) {
        this.N && this.N.A();
        this.L.ra(a)
    };

    function yz(a, b, c) {
        var d = a.L;
        b !== d.B && (d.B = b) && d.D.B !== b && (d = d.D, d.B = b, fp(d));
        a.Eg(c)
    }
    q.Ib = function(a, b) {
        var c = this.C.get();
        U(c, a);
        Dy(this.C, b)
    };

    function FE(a, b) {
        var c = a.width.get();
        a = a.height.get();
        b = ve(b);
        Be(b, c);
        De(b, a)
    }

    function RE(a, b) {
        FE(a, b);
        (a = a.C.get()) && zx(a) && (b.data[3] = 13.1)
    }
    q.Yk = function(a) {
        if (this.N) this.N.start(a);
        else {
            var b = this;
            this.N = new CC(300, function(a) {
                SE(b, a)
            }, "resize");
            SE(this, a)
        }
    };

    function SE(a, b) {
        var c = new oe;
        Be(c, a.width.get() || 0);
        De(c, a.height.get() || 0);
        a.Nm.L(c, b);
        if (a.A.Sa) {
            var d = a.A.Sa.A;
            a.Xd(!(d.drawingBufferWidth == d.canvas.width && d.drawingBufferHeight == d.canvas.height), b)
        }
        TE(a) && (d = a.S.get(), d = ue(d), d.W() != c.W() || Ce(d) != Ce(c)) && (d.W(), c.W(), U(ve(a.G.get()), c), a.B.Hg(c, b), tE(a.L, "resize"))
    }

    function TE(a) {
        return y(a.S.get()) && y(a.C.get())
    }

    function UE(a, b, c) {
        b.la("stableViewport");
        vE(a.L, function() {
            c(b);
            b.done("stableViewport")
        })
    }

    function VE(a, b, c, d, e, f) {
        var g = f || C;
        if (c && 4 === Xs(c)) g(e);
        else if (d = d ? d.D() : new Ex, a.xa || a.aa || a.X) g(e);
        else {
            var k = e.sa(function() {
                g(e)
            }, "moveTo", "mt0", "mt1");
            c && vo(e, "sc", "" + Xs(c));
            a.U += 1;
            f = TE(a);
            if (!f) {
                if (!c)
                    if (a.ja) c = a.ja;
                    else {
                        k();
                        return
                    }
                a.ja = yx(c);
                if (!b) {
                    WE(a, c, e);
                    k();
                    return
                }
            }
            var l = a.C.get(),
                m = a.S.get();
            f || l || (l = new Vs);
            f || m || (m = new le, RE(a, m));
            b && As(qe(m), qe(b)) && Bs(Ae(se(m)), Ae(se(b))) && Bs(N(se(m), 1), N(se(b), 1)) && Bs(N(se(m), 2), N(se(b), 2)) && Bs(pe(m), pe(b)) && (b = null);
            if (c) {
                var n = ux(l, c),
                    p = wx(l,
                        c),
                    r = Oc($s(l), $s(c));
                if (n || p && r) c = null
            }
            if (b || c || d.C) {
                b && (U(a.Ta, b), b = a.Ta, K(b, 2) || FE(a, b));
                c && (U(a.Ua, c), c = a.Ua);
                if (l && 2 === Xs(l) && 7 == L(Ys(l), 1, 99) || c && 2 === Xs(c) && 7 == L(Ys(c), 1, 99)) d.A = 2;
                !e.Wc() && l && c && K(l, 0) && K(c, 0) && Xs(l) != Xs(c) && e.ob("transitions", "switch_map_mode");
                a.F.A++;
                n = E(function() {
                    k();
                    EE(this.F)
                }, a);
                f && !d.C && a.B.Gg(b, c, d, e, n) || XE(a, b || m, c || l, f, d, e, n)
            } else k()
        }
    }

    function XE(a, b, c, d, e, f, g) {
        d && YE(a, a.H, null, null, !1, f);
        var k = a.U,
            l = yx(c),
            m = zs(b);
        a.Ma.load(c, E(function(a, b) {
            if (this.U === k) {
                this.X = !0;
                var c = e.B || !this.A.Sa,
                    n = this.C.get() || new Vs;
                U(n, l);
                YE(this, a, m, n, c, b);
                d || (this.F.H = !0, f.ua("scnd"), f.dispatchEvent("scnd"));
                this.X = !1
            }
            g(b)
        }, a), f)
    }

    function YE(a, b, c, d, e, f) {
        if (b !== a.B) {
            a.L.I = !1;
            var g = a.B;
            g.Wf(f);
            dl(a.Wa);
            a.B = b;
            b.Fg(c, d, f);
            a.Wa = Uo(a.B, "user-input-event", a.Ye);
            g !== a.H && (Cy(g.K, f), Cy(g.L, f), Cy(g.S, f), Cy(g.B, f));
            g !== a.H && Cy(a.ga, f);
            b !== a.H && (By(b.K, a.Um, f), By(b.L, a.G, f), By(b.S, a.S, f), By(b.B, a.C, f));
            d && WE(a, d, f);
            if (c) {
                var g = a.width.get(),
                    k = a.height.get();
                d = new oe;
                Be(d, g);
                De(d, k);
                c ? (b.Hc(c, d, e), FE(a, c), y(a.S.get()) ? (U(a.S.get(), c), Dy(a.S, f)) : a.S.set(zs(c), f), y(a.G.get()) || a.G.set(zs(c), f)) : (c = a.S.get(), b.Hc(c, d, e) && (FE(a, c), Dy(a.S,
                    f)))
            }
            b !== a.H && b.nf(a.ga, f);
            b.wb(a, f);
            Dy(a.Zb, f);
            Dy(a.Pb, f);
            Dy(a.Fb, f);
            b !== a.H ? a.D.start(f) : a.D.A();
            "application_init" == f.C && !f.Nc.drv && vo(f, "drv", b.Ge())
        }
    }

    function WE(a, b, c) {
        y(a.C.get()) ? (U(a.C.get(), b), Dy(a.C, c)) : a.C.set(yx(b), c)
    }

    function ZE(a, b) {
        a.Y && (a.Y(a.Oe, b), a.Y = null)
    }
    q.Ed = function(a, b, c, d) {
        this.B.G && (b.Wc() || b.ob("scene", "scroll_zoom"), this.B.G.Ed(a, b, c, d))
    };

    function $E(a, b) {
        ZE(a, b.A);
        var c = a.C.get();
        if (!c || zx(c)) return !1;
        var c = b.A,
            d = b.B;
        d.ob("scene", "click_scene");
        aF(a, c.x, c.y, a.J, d, b.D);
        a.B.Se(eE(c), d);
        return !0
    }

    function bF(a, b) {
        var c = b.A,
            d = b.B;
        aF(a, c.x, c.y, a.J, d, b.D);
        if (a.B.I) {
            c = eE(c);
            a = a.B.I;
            b = c.x;
            var c = c.y,
                e = d.event();
            e && ez(a, e);
            fz(a.A, b, c, d)
        }
    }
    q.Sk = function(a, b) {
        this.B.Rg(eE(b), a)
    };
    q.Vk = function(a, b) {
        this.B.Ug(eE(b), a)
    };
    q.Tk = function(a, b) {
        if (this.B.I) {
            var c = this.B.I;
            b = eE(b);
            ez(c, b);
            Nx(c.A.A, !0);
            fz(c.A, b.x, b.y, a)
        }
    };
    q.Uk = function(a, b) {
        this.B.I && (a = this.B.I, ez(a, eE(b)), Nx(a.A.A, !1))
    };
    q.jl = function(a, b) {
        if (!(b.Ji && !this.Ec() || Math.abs(b.Cd) < Math.abs(b.ef) || 0 === b.Cd)) {
            ZE(this, b);
            aF(this, b.x, b.y, this.J, a);
            document.body.focus();
            var c;
            this.Ec() ? (c = b.Lk, 1 < Math.abs(c) && (c = Gb(0 > c ? -1 : 1, c, QE)), c = b.ctrlKey ? -c : -c / 4) : c = 0 >= b.Cd ? 1 : -1;
            this.Gc(c, a, b.x, b.y, !0)
        }
    };
    q.Lg = function(a, b) {
        if ((!(b.ctrlKey || b.metaKey || b.altKey) || b.shiftKey) && this.B.P) {
            var c = this.B.P;
            if (!c.F) switch (b.keyCode) {
                case 38:
                case 87:
                    uz(c, !0, a, 3);
                    break;
                case 40:
                case 83:
                    uz(c, !1, a, 4);
                    break;
                case 37:
                case 65:
                    wz(c, !0, !0, a);
                    break;
                case 39:
                case 68:
                    wz(c, !1, !0, a);
                    break;
                case 107:
                case 187:
                    c.Gc(1, a, void 0, void 0, !0);
                    c.Ga.D(by, 32);
                    break;
                case 109:
                case 189:
                    c.Gc(-1, a, void 0, void 0, !0), c.Ga.D(cy, 32)
            }
        }
    };
    q.Mg = function(a, b) {
        if (this.B.P) {
            var c = this.B.P;
            37 != b.keyCode && 39 != b.keyCode && 65 != b.keyCode && 68 != b.keyCode || !c.C || c.C.cancel(a)
        }
    };

    function aF(a, b, c, d, e, f) {
        a.O = b;
        a.P = c;
        a.J = d;
        cF(a, e, f)
    }

    function cF(a, b, c) {
        a.C.get();
        var d = a.B.na,
            e = a.B.Te(a.O, a.P, b);
        a.B.Tg(a.O, a.P, a.J, b) ? a.I.set("move", b) : e ? a.I.set("pointer", b) : y(c) || d ? ("pointer" !== a.I.get() && a.I.set("auto", b), 0 == a.F.A && (y(c) ? dF(a, c, b) : dF(a, JD(d, a.O, a.P), b))) : a.I.set("auto", b)
    }

    function dF(a, b, c) {
        b && b.B() ? a.I.set("pointer", c) : a.I.set("auto", c)
    }
    q.Gc = function(a, b, c, d, e) {
        this.B.G && (b.Wc() || b.ob("scene", "scroll_zoom"), this.B.G.Gc(a, b, c, d, e))
    };

    function eF(a, b, c) {
        if (a.B.xa) return eF(a.B.xa, b, c);
        a = c || new GE(0, 0);
        a.x = 0;
        a.y = 0;
        return a
    }
    q.Ec = function() {
        return this.B.G ? this.B.G.Ec() : !1
    };
    q.rk = function(a) {
        var b = this.C.get();
        if (b && zx(b) && (b = this.S.get()) && K(b, 0)) {
            var c = qe(b);
            if (K(c, 2) && K(c, 1) && K(c, 0)) {
                var d = xe(c); - 90 > d || 90 < d || isNaN(d) || (d = we(c), -180 > d || 180 < d || isNaN(d) || (c = ye(c), -10898 > c || isNaN(c) || !K(b, 3) || (c = pe(b), 1 > c || 179 < c || isNaN(c) || !K(b, 2) || (b = ue(b), K(b, 0) && K(b, 1) && (1 > b.W() || 1 > Ce(b) || b.W())))))
            }
        }
        b = this.wa;
        b.B || b.C || this.da || (this.D.A(), this.D.start(a));
        cF(this, a)
    };
    q.Eg = function(a) {
        var b = this.L.B;
        if (b) {
            var c = this.ha.get();
            c && (b.D.yc(c.top, c.right, c.bottom, c.left, a), KD(b))
        }
    };

    function dz(a, b, c, d) {
        a = a.wa;
        d = c.sa(E(a.G, a, d), "animation-");
        a.B = new DE(a.A, b, a.D, d, c);
        b = a.A;
        b.D.A();
        b.F.A++;
        CE(a.B);
        return a.B
    }

    function az(a, b, c, d, e) {
        var f = a.S.get(),
            g = a.C.get();
        b = new Fx(f, g, b, c);
        YE(a, a.H, null, null, !1, d);
        yz(a, null, d);
        cF(a, d);
        a.Qm.load(b, d, function(b, c) {
            a.$k.call(a, b, e, c)
        })
    }
    q.$k = function(a, b, c) {
        var d = this;
        this.aa = !0;
        AE(this.wa, a, c, function(a) {
            var c = d.S.get(),
                e = d.C.get();
            d.Ma.load(e, function(a, f) {
                d.aa = !1;
                YE(d, a, c, e, !1, f);
                b(f)
            }, a)
        })
    };

    function fF(a) {
        this.D = a;
        gF || (gF = !0, a = a.style || void 0, On("transformOrigin", a), hF = On("transform", a) || "transform")
    }
    var gF = !1,
        hF = "",
        iF = new Float64Array(2);

    function jF(a) {
        var b = w.devicePixelRatio || 1;
        return Math.round(a * b) / b
    }
    fF.prototype.detach = function(a) {
        a.parentNode === this.D && Yj(a)
    };

    function kF(a) {
        for (var b = a.D.firstChild; b; b = b.nextSibling)
            if (1 === b.nodeType) {
                var c = a,
                    d = b,
                    e = d.__tai;
                if (!e || !e.fixed) {
                    var f;
                    f = e.tn;
                    (c = c.A.get()) ? (lF.data[1] = f[0], lF.data[2] = f[1], ze(lF, f[2]), eF(c, lF, mF), iF[0] = mF.x, iF[1] = mF.y, f = !0) : f = !1;
                    f ? (e = e.gn, d.style.display = "block", d.style[hF] = "translateZ(0) translate(" + jF(iF[0] - e[0]) + "px," + jF(iF[1] - e[1]) + "px) scale(" + e[2] + ")") : d.style.display = "none"
                }
            }
    };

    function nF(a) {
        fF.call(this, a);
        this.B = Gy(Iy);
        this.A = Gy(HE);
        this.C = !1
    }
    H(nF, fF);
    var lF = new me,
        mF = new GE(0, 0);
    nF.prototype.bind = function(a, b, c) {
        By(this.B, a, c);
        By(this.A, b, c);
        this.B.listen(this.F, this);
        this.A.listen(this.G, this)
    };
    nF.prototype.F = function(a) {
        if (this.B.get() && this.A.get() && !this.C) {
            this.C = !0;
            var b = E(function() {
                this.C = !1;
                kF(this)
            }, this);
            Ny(b, 0, a, "effect-surface-camera-update")
        }
    };
    nF.prototype.G = function() {
        this.B.get() && this.A.get() && !this.C && kF(this)
    };

    function oF(a, b, c) {
        YD.call(this, c);
        this.D = a;
        this.A = b;
        this.B = aE(this);
        this.C = $D(this)
    }
    H(oF, YD);
    oF.prototype.N = function(a) {
        var b = this.B.get() || "";
        this.A.data[0] = b;
        b = !!this.C.get();
        this.A.data[2] = b;
        this.D.fill(this.A);
        dE(this.D, a.sa(C, "scene.template-render"))
    };
    oF.prototype.L = C;

    function pF(a, b, c, d) {
        this.F = a;
        this.J = b;
        this.I = c;
        this.K = d;
        this.B = !1;
        this.C = [];
        this.A = {};
        this.H = {};
        this.G = {};
        this.M = 0
    }

    function qF(a, b, c, d, e) {
        var f = e ? e : null,
            g = a.A[b];
        g || (g = {}, a.A[b] = g);
        var k = g[f];
        e = !!k;
        k || (k = [], g[f] = k);
        b = new rF(b, f, c, d);
        k.push(b);
        c = a.M++;
        a.H[c] = b;
        return e
    }

    function sF(a, b, c, d, e, f) {
        f || "drag" != b && "dragstart" != b && "dragend" != b || (f = 0, f = Fk ? f : Jk[f]);
        qF(a, b, c, d, f) || (c = f ? f : null, d = E(a.D, a, b, c), e = e ? a.F.Qb(b, null, d, f) : a.F.wc(a.J, a.I, b, null, d, f), f = a.G[b], f || (f = {}, a.G[b] = f), f[c] = e)
    }
    pF.prototype.D = function(a, b, c, d) {
        c.la("scene-async-event-handler");
        if (!this.B) {
            this.B = !0;
            var e = this.K;
            e.J.push(this);
            ZD(e.B)
        }
        "scrollwheel" == a && y(d.Cd) && y(d.ef) && Math.abs(d.Cd) >= Math.abs(d.ef) && co(d);
        this.C.push(new tF(a, b, d, c))
    };

    function tF(a, b, c, d) {
        this.type = a;
        this.qualifier = b;
        this.event = c;
        this.oa = d
    }

    function rF(a, b, c, d) {
        this.rb = a;
        this.qualifier = b;
        this.A = c;
        this.gb = d
    };

    function uF(a) {
        this.data = a || []
    }
    H(uF, J);

    function vF(a) {
        this.data = a || {}
    }
    H(vF, Bc);

    function wF(a) {
        this.data = a || {}
    }
    H(wF, Bc);

    function xF(a) {
        this.data = a || {}
    }
    H(xF, Bc);

    function yF(a) {
        zF.data.css3_prefix = a
    };
    var AF = {};

    function BF() {
        this.A = {};
        this.B = null;
        this.Xa = ++CF
    }
    var DF = 0,
        CF = 0;

    function EF() {
        zF || (zF = new xF, Za() && !Yb("Edge") ? yF("-webkit-") : Yb("Firefox") ? yF("-moz-") : zc() ? yF("-ms-") : Yb("Opera") && yF("-o-"), zF.data.client_platform = 3, zF.data.is_rtl = !1);
        return zF
    }
    var zF = null;

    function FF() {
        return EF().data
    }

    function GF(a, b, c) {
        return b.call(c, a.A, AF)
    }

    function HF(a, b, c) {
        null != b.B && (a.B = b.B);
        a = a.A;
        b = b.A;
        if (c = c || null) {
            a.Fa = b.Fa;
            a.Ya = b.Ya;
            for (var d = 0; d < c.length; ++d) a[c[d]] = b[c[d]]
        } else
            for (d in b) a[d] = b[d]
    };

    function IF(a, b) {
        this.A = "";
        this.B = b || {};
        if ("string" === typeof a) this.A = a;
        else {
            b = a.B;
            this.A = a.A;
            for (var c in b) null == this.B[c] && (this.B[c] = b[c])
        }
    }

    function JF(a) {
        return a.A
    }

    function KF(a) {
        if (!a) return LF();
        for (a = a.parentNode; za(a) && 1 == a.nodeType; a = a.parentNode) {
            var b = a.getAttribute("dir");
            if (b && (b = b.toLowerCase(), "ltr" == b || "rtl" == b)) return b
        }
        return LF()
    }

    function MF(a) {
        for (var b = 0; b < arguments.length; ++b)
            if (!arguments[b]) return !1;
        return !0
    }

    function NF(a, b) {
        return a > b
    }

    function OF(a, b) {
        return a < b
    }

    function PF(a, b) {
        return a >= b
    }

    function QF(a, b) {
        return a <= b
    }

    function RF(a) {
        return "string" == typeof a ? "'" + a.replace(/\'/g, "\\'") + "'" : String(a)
    }

    function SF(a) {
        return null != a && "object" == typeof a && "number" == typeof a.length && "undefined" != typeof a.propertyIsEnumerable && !a.propertyIsEnumerable("length")
    }

    function TF(a, b, c) {
        for (var d = 2; d < arguments.length; ++d) {
            if (null == a || null == arguments[d]) return b;
            var e = arguments[d];
            if ("number" == typeof e && 0 > e)
                if (null == a.length) a = a[-e];
                else {
                    var e = -e - 1,
                        f = a[e];
                    null == f || za(f) && !SF(f) ? (f = a[a.length - 1], e = SF(f) || !za(f) ? null : f[e + 1] || null) : e = f;
                    a = e
                }
            else a = a[e]
        }
        return null == a ? b : a
    }

    function UF(a, b, c) {
        c = ~~(c || 0);
        0 == c && (c = 1);
        var d = [];
        if (0 < c)
            for (a = ~~a; a < b; a += c) d.push(a);
        else
            for (a = ~~a; a > b; a += c) d.push(a);
        return d
    }

    function LF() {
        var a = EF();
        return Cc(a, "is_rtl", void 0) ? "rtl" : "ltr"
    }

    function VF(a, b, c) {
        switch (pi(a, b)) {
            case 1:
                return "ltr";
            case -1:
                return "rtl";
            default:
                return c
        }
    }

    function WF(a, b, c) {
        switch (pi(a, b)) {
            case 1:
                return !1;
            case -1:
                return !0;
            default:
                return c
        }
    }

    function XF(a, b, c) {
        return YF(a, b, "rtl" == c) ? "rtl" : "ltr"
    }

    function YF(a, b, c) {
        return c ? !li.test(hi(a, b)) : mi.test(hi(a, b))
    }
    var ZF = /[\'\"\(]/,
        $F = ["border-color", "border-style", "border-width", "margin", "padding"],
        aG = /left/g,
        bG = /right/g,
        cG = /\s+/;

    function dG(a, b) {
        if (ZF.test(b)) return b;
        b = 0 <= b.indexOf("left") ? b.replace(aG, "right") : b.replace(bG, "left");
        sb($F, a) && (a = b.split(cG), 4 <= a.length && (b = [a[0], a[3], a[2], a[1]].join(" ")));
        return b
    }

    function eG(a) {
        if (null != a) {
            var b = a.ordinal;
            null == b && (b = a.cl);
            if (null != b && "function" == typeof b) return String(b.call(a))
        }
        return "" + a
    }

    function fG(a) {
        if (null == a) return 0;
        var b = a.ordinal;
        null == b && (b = a.cl);
        return null != b && "function" == typeof b ? b.call(a) : 0 <= a ? Math.floor(a) : Math.ceil(a)
    }

    function gG(a) {
        try {
            return void 0 !== a.call(null)
        } catch (b) {
            return !1
        }
    }

    function hG(a) {
        try {
            var b = a.call(null);
            return SF(b) ? b.length : void 0 === b ? 0 : 1
        } catch (c) {
            return 0
        }
    }

    function iG(a, b) {
        return null == a ? null : new IF(a, b)
    }

    function jG(a) {
        if (null != a.data.original_value) {
            var b = new lr(Dc(a, "original_value"));
            "original_value" in a.data && delete a.data.original_value;
            b.B && (a.data.protocol = b.B);
            b.C && (a.data.host = b.C);
            null != b.H ? a.data.port = b.H : b.B && ("http" == b.B ? a.data.port = 80 : "https" == b.B && (a.data.port = 443));
            b.D && (a.data.path = b.D);
            b.F && (a.data.hash = b.F);
            for (var c = b.A.kb(), d = 0; d < c.length; ++d) {
                var e = c[d],
                    f = new vF(Ec(a));
                f.data.key = e;
                e = b.A.Ca(e)[0];
                f.data.value = e
            }
        }
    }

    function kG(a, b) {
        var c;
        "string" == typeof a ? (c = new wF, c.data.original_value = a) : c = new wF(a);
        jG(c);
        if (b)
            for (a = 0; a < b.length; ++a) {
                for (var d = b[a], e = null != d.key ? d.key : d.key, f = null != d.value ? d.value : d.value, d = !1, g = 0; g < Gc(c); ++g)
                    if (Dc(new vF(Fc(c, g)), "key") == e) {
                        (new vF(Fc(c, g))).data.value = f;
                        d = !0;
                        break
                    }
                d || (d = new vF(Ec(c)), d.data.key = e, d.data.value = f)
            }
        return c.data
    }

    function lG(a) {
        a = new wF(a);
        jG(a);
        var b = null != a.data.protocol ? Dc(a, "protocol") : null,
            c = null != a.data.host ? Dc(a, "host") : null,
            d = null != a.data.port && (null == a.data.protocol || "http" == Dc(a, "protocol") && 80 != Cc(a, "port", 0) || "https" == Dc(a, "protocol") && 443 != Cc(a, "port", 0)) ? Cc(a, "port", 0) : null,
            e = null != a.data.path ? Dc(a, "path") : null,
            f = null != a.data.hash ? Dc(a, "hash") : null,
            g = new lr(null, void 0);
        b && mr(g, b);
        c && (g.C = c);
        d && nr(g, d);
        e && (g.D = e);
        f && (g.F = f);
        for (b = 0; b < Gc(a); ++b) c = new vF(Fc(a, b)), g.A.set(Dc(c, "key"), Dc(c, "value"));
        return g.toString()
    }

    function mG(a, b) {
        a = new wF(a);
        jG(a);
        for (var c = 0; c < Gc(a); ++c) {
            var d = new vF(Fc(a, c));
            if (Dc(d, "key") == b) return Dc(d, "value")
        }
        return ""
    }

    function nG(a, b) {
        a = new wF(a);
        jG(a);
        for (var c = 0; c < Gc(a); ++c)
            if (Dc(new vF(Fc(a, c)), "key") == b) return !0;
        return !1
    }

    function oG(a) {
        return null != a && a.ib ? a.ib() : a
    };
    var pG = /^<\/?(b|u|i|em|br|sub|sup|wbr|span)( dir=(rtl|ltr|'ltr'|'rtl'|"ltr"|"rtl"))?>/,
        qG = /^&([a-zA-Z]+|#[0-9]+|#x[0-9a-fA-F]+);/,
        rG = {
            "<": "&lt;",
            ">": "&gt;",
            "&": "&amp;",
            '"': "&quot;"
        };

    function sG(a) {
        if (null == a) return "";
        if (!tG.test(a)) return a; - 1 != a.indexOf("&") && (a = a.replace(uG, "&amp;")); - 1 != a.indexOf("<") && (a = a.replace(vG, "&lt;")); - 1 != a.indexOf(">") && (a = a.replace(wG, "&gt;")); - 1 != a.indexOf('"') && (a = a.replace(xG, "&quot;"));
        return a
    }

    function yG(a) {
        if (null == a) return ""; - 1 != a.indexOf('"') && (a = a.replace(xG, "&quot;"));
        return a
    }
    var uG = /&/g,
        vG = /</g,
        wG = />/g,
        xG = /\"/g,
        tG = /[&<>\"]/,
        zG = null;

    function AG(a) {
        for (var b = "", c = 0, d; d = a[c]; ++c) switch (d) {
            case "<":
            case "&":
                var e = ("<" == d ? pG : qG).exec(a.substr(c));
                if (e && e[0]) {
                    b += a.substr(c, e[0].length);
                    c += e[0].length - 1;
                    continue
                }
            case ">":
            case '"':
                b += rG[d];
                break;
            default:
                b += d
        }
        null == zG && (zG = document.createElement("div"));
        zG.innerHTML = b;
        return zG.innerHTML
    };

    function BG(a) {
        var b = a.length - 1,
            c = null;
        switch (a[b]) {
            case "filter_url":
                c = 1;
                break;
            case "filter_imgurl":
                c = 2;
                break;
            case "filter_css_regular":
                c = 5;
                break;
            case "filter_css_string":
                c = 6;
                break;
            case "filter_css_url":
                c = 7
        }
        c && ub(a, b);
        return c
    }

    function CG(a) {
        if (DG.test(a)) return a;
        a = Gi(a).lb();
        return "about:invalid#zClosurez" === a ? "about:invalid#zjslayoutz" : a
    }
    var DG = /^data:image\/(?:bmp|gif|jpeg|jpg|png|tiff|webp);base64,[-+/_a-z0-9]+(?:=|%3d)*$/i;

    function EG(a) {
        var b = FG.exec(a);
        if (!b) return "0;url=about:invalid#zjslayoutz";
        var c = b[2];
        return b[1] ? "about:invalid#zClosurez" == Gi(c).lb() ? "0;url=about:invalid#zjslayoutz" : a : 0 == c.length ? a : "0;url=about:invalid#zjslayoutz"
    }
    var FG = /^(?:[0-9]+)([ ]*;[ ]*url=)?(.*)$/;

    function GG(a) {
        if (null == a) return null;
        if (!HG.test(a) || 0 != IG(a, 0)) return "zjslayoutzinvalid";
        for (var b = /([-_a-zA-Z0-9]+)\(/g, c; null !== (c = b.exec(a));)
            if (null === JG(c[1], !1)) return "zjslayoutzinvalid";
        return a
    }

    function IG(a, b) {
        if (0 > b) return -1;
        for (var c = 0; c < a.length; c++) {
            var d = a.charAt(c);
            if ("(" == d) b++;
            else if (")" == d)
                if (0 < b) b--;
                else return -1
        }
        return b
    }

    function KG(a) {
        if (null == a) return null;
        for (var b = /([-_a-zA-Z0-9]+)\(/g, c = /[ \t]*((?:"(?:[^\x00"\\\n\r\f\u0085\u000b\u2028\u2029]*)"|'(?:[^\x00'\\\n\r\f\u0085\u000b\u2028\u2029]*)')|(?:[?&/:=]|[+\-.,!#%_a-zA-Z0-9\t])*)[ \t]*/g, d = !0, e = 0, f = ""; d;) {
            b.lastIndex = 0;
            var g = b.exec(a),
                d = null !== g,
                k = a;
            if (d) {
                if (void 0 === g[1]) return "zjslayoutzinvalid";
                var l = JG(g[1], !0);
                if (null === l) return "zjslayoutzinvalid";
                k = a.substring(0, b.lastIndex);
                a = a.substring(b.lastIndex)
            }
            e = IG(k, e);
            if (0 > e || !HG.test(k)) return "zjslayoutzinvalid";
            f += k;
            if (d && "url" == l) {
                c.lastIndex = 0;
                g = c.exec(a);
                if (null === g || 0 != g.index) return "zjslayoutzinvalid";
                var m = g[1];
                if (void 0 === m) return "zjslayoutzinvalid";
                g = 0 == m.length ? 0 : c.lastIndex;
                if (")" != a.charAt(g)) return "zjslayoutzinvalid";
                k = "";
                1 < m.length && (0 == m.lastIndexOf('"', 0) && Ja(m, '"') ? (m = m.substring(1, m.length - 1), k = '"') : 0 == m.lastIndexOf("'", 0) && Ja(m, "'") && (m = m.substring(1, m.length - 1), k = "'"));
                m = CG(m);
                if ("about:invalid#zjslayoutz" == m) return "zjslayoutzinvalid";
                f += k + m + k;
                a = a.substring(g)
            }
        }
        return 0 != e ? "zjslayoutzinvalid" :
            f
    }

    function JG(a, b) {
        var c = a.toLowerCase();
        a = LG.exec(a);
        if (null !== a) {
            if (void 0 === a[1]) return null;
            c = a[1]
        }
        return b && "url" == c || c in MG ? c : null
    }
    var MG = {
            blur: !0,
            brightness: !0,
            calc: !0,
            circle: !0,
            contrast: !0,
            counter: !0,
            counters: !0,
            "cubic-bezier": !0,
            "drop-shadow": !0,
            ellipse: !0,
            grayscale: !0,
            hsl: !0,
            hsla: !0,
            "hue-rotate": !0,
            inset: !0,
            invert: !0,
            opacity: !0,
            "linear-gradient": !0,
            matrix: !0,
            matrix3d: !0,
            polygon: !0,
            "radial-gradient": !0,
            rgb: !0,
            rgba: !0,
            rect: !0,
            rotate: !0,
            rotate3d: !0,
            rotatex: !0,
            rotatey: !0,
            rotatez: !0,
            saturate: !0,
            sepia: !0,
            scale: !0,
            scale3d: !0,
            scalex: !0,
            scaley: !0,
            scalez: !0,
            steps: !0,
            skew: !0,
            skewx: !0,
            skewy: !0,
            translate: !0,
            translate3d: !0,
            translatex: !0,
            translatey: !0,
            translatez: !0
        },
        HG = /^(?:[*/]?(?:(?:[+\-.,!#%_a-zA-Z0-9\t]| )|\)|[a-zA-Z0-9]\(|$))*$/,
        NG = /^(?:[*/]?(?:(?:"(?:[^\x00"\\\n\r\f\u0085\u000b\u2028\u2029]|\\(?:[\x21-\x2f\x3a-\x40\x47-\x60\x67-\x7e]|[0-9a-fA-F]{1,6}[ \t]?))*"|'(?:[^\x00'\\\n\r\f\u0085\u000b\u2028\u2029]|\\(?:[\x21-\x2f\x3a-\x40\x47-\x60\x67-\x7e]|[0-9a-fA-F]{1,6}[ \t]?))*')|(?:[+\-.,!#%_a-zA-Z0-9\t]| )|$))*$/,
        LG = /^-(?:moz|ms|o|webkit|css3)-(.*)$/;

    function OG(a, b) {
        var c = a.__innerhtml;
        c || (c = a.__innerhtml = [a.innerHTML, a.innerHTML]);
        if (c[0] != b || c[1] != a.innerHTML) a.innerHTML = b, c[0] = b, c[1] = a.innerHTML
    }
    var PG = {
        action: !0,
        cite: !0,
        data: !0,
        formaction: !0,
        href: !0,
        icon: !0,
        manifest: !0,
        poster: !0,
        src: !0
    };

    function QG(a) {
        if (a = a.getAttribute("jsinstance")) {
            var b = a.indexOf(";");
            return (0 <= b ? a.substr(0, b) : a).split(",")
        }
        return []
    }

    function RG(a) {
        if (a = a.getAttribute("jsinstance")) {
            var b = a.indexOf(";");
            return 0 <= b ? a.substr(b + 1) : null
        }
        return null
    }

    function SG(a, b, c) {
        var d = a[c] || "0",
            e = b[c] || "0",
            d = parseInt("*" == d.charAt(0) ? d.substring(1) : d, 10),
            e = parseInt("*" == e.charAt(0) ? e.substring(1) : e, 10);
        return d == e ? a.length > c || b.length > c ? SG(a, b, c + 1) : !1 : d > e
    }

    function TG(a, b, c, d, e, f) {
        b[c] = e >= d - 1 ? "*" + e : String(e);
        b = b.join(",");
        f && (b += ";" + f);
        a.setAttribute("jsinstance", b)
    }

    function UG(a) {
        if (!a.hasAttribute("jsinstance")) return a;
        for (var b = QG(a);;) {
            var c = bk(a);
            if (!c) return a;
            var d = QG(c);
            if (!SG(d, b, 0)) return a;
            a = c;
            b = d
        }
    };
    var VG = {
            "for": "htmlFor",
            "class": "className"
        },
        WG = {},
        XG;
    for (XG in VG) WG[VG[XG]] = XG;
    var YG = {
        9: 1,
        11: 3,
        10: 4,
        12: 5,
        13: 6,
        14: 7
    };

    function ZG(a, b, c, d) {
        if (null == a[1]) {
            var e = a[1] = a[0].match(ir);
            if (e[6]) {
                for (var f = e[6].split("&"), g = {}, k = 0, l = f.length; k < l; ++k) {
                    var m = f[k].split("=");
                    if (2 == m.length) {
                        var n = m[1].replace(/,/gi, "%2C").replace(/[+]/g, "%20").replace(/:/g, "%3A");
                        try {
                            g[decodeURIComponent(m[0])] = decodeURIComponent(n)
                        } catch (p) {}
                    }
                }
                e[6] = g
            }
            a[0] = null
        }
        a = a[1];
        b in YG && (e = YG[b], 13 == b ? c && (b = a[e], null != d ? (b || (b = a[e] = {}), b[c] = d) : b && delete b[c]) : a[e] = d)
    };

    function $G(a) {
        this.H = a;
        this.G = this.F = this.C = this.A = null;
        this.I = this.D = 0;
        this.K = !1;
        this.B = -1;
        this.ta = ++aH
    }
    $G.prototype.name = h("H");

    function bH(a, b) {
        return "href" == b.toLowerCase() ? "#" : "img" == a.toLowerCase() && "src" == b.toLowerCase() ? "/images/cleardot.gif" : ""
    }
    $G.prototype.id = h("ta");
    var aH = 0;

    function cH(a) {
        a.C = a.A;
        a.A = a.C.slice(0, a.B);
        a.B = -1
    }

    function dH(a) {
        for (var b = (a = a.A) ? a.length : 0, c = 0; c < b; c += 7)
            if (0 == a[c + 0] && "dir" == a[c + 1]) return a[c + 5];
        return null
    }

    function eH(a, b, c, d, e, f, g, k) {
        var l = a.B;
        if (-1 != l) {
            if (a.A[l + 0] == b && a.A[l + 1] == c && a.A[l + 2] == d && a.A[l + 3] == e && a.A[l + 4] == f && a.A[l + 5] == g && a.A[l + 6] == k) {
                a.B += 7;
                return
            }
            cH(a)
        } else a.A || (a.A = []);
        a.A.push(b);
        a.A.push(c);
        a.A.push(d);
        a.A.push(e);
        a.A.push(f);
        a.A.push(g);
        a.A.push(k)
    }

    function fH(a, b) {
        a.D |= b
    }

    function gH(a) {
        return a.D & 1024 ? (a = dH(a), "rtl" == a ? "\u202c\u200e" : "ltr" == a ? "\u202c\u200f" : "") : !1 === a.G ? "" : "</" + a.H + ">"
    }

    function hH(a, b, c, d) {
        for (var e = -1 != a.B ? a.B : a.A ? a.A.length : 0, f = 0; f < e; f += 7)
            if (a.A[f + 0] == b && a.A[f + 1] == c && a.A[f + 2] == d) return !0;
        if (a.F)
            for (f = 0; f < a.F.length; f += 7)
                if (a.F[f + 0] == b && a.F[f + 1] == c && a.F[f + 2] == d) return !0;
        return !1
    }
    $G.prototype.reset = function(a) {
        if (!this.K && (this.K = !0, this.B = -1, null != this.A)) {
            for (var b = 0; b < this.A.length; b += 7)
                if (this.A[b + 6]) {
                    var c = this.A.splice(b, 7),
                        b = b - 7;
                    this.F || (this.F = []);
                    Array.prototype.push.apply(this.F, c)
                }
            this.I = 0;
            if (a)
                for (b = 0; b < this.A.length; b += 7)
                    if (c = this.A[b + 5], -1 == this.A[b + 0] && c == a) {
                        this.I = b;
                        break
                    }
            0 == this.I ? this.B = 0 : this.C = this.A.splice(this.I, this.A.length)
        }
    };

    function iH(a, b, c, d, e, f) {
        if (6 == b) {
            if (d)
                for (e && (d = Va(d)), b = d.split(" "), c = b.length, d = 0; d < c; d++) "" != b[d] && jH(a, 7, "class", b[d], "", f)
        } else 18 != b && 20 != b && 22 != b && hH(a, b, c) || eH(a, b, c, null, null, e || null, d, !!f)
    }

    function kH(a, b, c, d, e) {
        var f;
        switch (b) {
            case 2:
            case 1:
                f = 8;
                break;
            case 8:
                f = 0;
                d = EG(d);
                break;
            default:
                f = 0, d = "sanitization_error_" + b
        }
        hH(a, f, c) || eH(a, f, c, null, b, null, d, !!e)
    }

    function jH(a, b, c, d, e, f) {
        switch (b) {
            case 5:
                c = "style"; - 1 != a.B && "display" == d && cH(a);
                break;
            case 7:
                c = "class"
        }
        hH(a, b, c, d) || eH(a, b, c, d, null, null, e, !!f)
    }

    function lH(a, b) {
        return b.toUpperCase()
    }

    function mH(a, b) {
        null === a.G ? a.G = b : a.G && !b && null != dH(a) && (a.H = "span")
    }

    function nH(a, b, c) {
        var d;
        if (c[1]) {
            d = c[1];
            if (d[6]) {
                var e = d[6],
                    f = [],
                    g;
                for (g in e) {
                    var k = e[g];
                    null != k && f.push(encodeURIComponent(g) + "=" + encodeURIComponent(k).replace(/%3A/gi, ":").replace(/%20/g, "+").replace(/%2C/gi, ",").replace(/%7C/gi, "|"))
                }
                d[6] = f.join("&")
            }
            "http" == d[1] && "80" == d[4] && (d[4] = null);
            "https" == d[1] && "443" == d[4] && (d[4] = null);
            e = d[3];
            /:[0-9]+$/.test(e) && (f = e.lastIndexOf(":"), d[3] = e.substr(0, f), d[4] = e.substr(f + 1));
            e = d[1];
            f = d[2];
            g = d[3];
            var k = d[4],
                l = d[5],
                m = d[6];
            d = d[7];
            var n = "";
            e && (n += e + ":");
            g &&
                (n += "//", f && (n += f + "@"), n += g, k && (n += ":" + k));
            l && (n += l);
            m && (n += "?" + m);
            d && (n += "#" + d);
            d = n
        } else d = c[0];
        (c = oH(c[2], d)) || (c = bH(a.H, b));
        return c
    }

    function pH(a, b, c) {
        if (a.D & 1024) return a = dH(a), "rtl" == a ? "\u202b" : "ltr" == a ? "\u202a" : "";
        if (!1 === a.G) return "";
        for (var d = "<" + a.H, e = null, f = "", g = null, k = null, l = "", m, n = "", p = "", r = 0 != (a.D & 832) ? "" : null, u = "", t = a.A, v = t ? t.length : 0, x = 0; x < v; x += 7) {
            var D = t[x + 0],
                z = t[x + 1],
                A = t[x + 2],
                B = t[x + 5],
                Q = t[x + 3],
                M = t[x + 6];
            if (null != B && null != r && !M) switch (D) {
                case -1:
                    r += B + ",";
                    break;
                case 7:
                case 5:
                    r += D + "." + A + ",";
                    break;
                case 13:
                    r += D + "." + z + "." + A + ",";
                    break;
                case 18:
                case 20:
                    break;
                default:
                    r += D + "." + z + ","
            }
            switch (D) {
                case 7:
                    null === B ? null != k && tb(k, A) :
                        null != B && (null == k ? k = [A] : sb(k, A) || k.push(A));
                    break;
                case 4:
                    m = !1;
                    g = Q;
                    null == B ? f = null : "" == f ? f = B : ";" == B.charAt(B.length - 1) ? f = B + f : f = B + ";" + f;
                    break;
                case 5:
                    m = !1;
                    null != B && null !== f && ("" != f && ";" != f[f.length - 1] && (f += ";"), f += A + ":" + B);
                    break;
                case 8:
                    null == e && (e = {});
                    null === B ? e[z] = null : B ? ((D = t[x + 4]) && (B = Va(B)), e[z] = [B, null, Q]) : e[z] = ["", null, Q];
                    break;
                case 18:
                    null != B && ("jsl" == z ? (m = !0, l += B) : "jsvs" == z && (n += B));
                    break;
                case 20:
                    null != B && (p && (p += ","), p += B);
                    break;
                case 22:
                    null != B && (u && (u += ";"), u += B);
                    break;
                case 21:
                case 0:
                    null !=
                        B && (d += " " + z + "=", B = oH(Q, B), d = (D = t[x + 4]) ? d + ('"' + yG(B) + '"') : d + ('"' + sG(B) + '"'));
                    break;
                case 14:
                case 11:
                case 12:
                case 10:
                case 9:
                case 13:
                    null == e && (e = {}), Q = e[z], null !== Q && (Q || (Q = e[z] = ["", null, null]), ZG(Q, D, A, B))
            }
        }
        if (null != e)
            for (z in e) t = nH(a, z, e[z]), d += " " + z + '="' + sG(t) + '"';
        u && (d += ' jsaction="' + yG(u) + '"');
        p && (d += ' jsinstance="' + sG(p) + '"');
        null != k && 0 < k.length && (d += ' class="' + sG(k.join(" ")) + '"');
        l && !m && (d += ' jsl="' + sG(l) + '"');
        if (null != f) {
            for (;
                "" != f && ";" == f[f.length - 1];) f = f.substr(0, f.length - 1);
            "" != f && (f = oH(g,
                f), d += ' style="' + sG(f) + '"')
        }
        l && m && (d += ' jsl="' + sG(l) + '"');
        n && (d += ' jsvs="' + sG(n) + '"');
        null != r && -1 != r.indexOf(".") && (d += ' jsan="' + r.substr(0, r.length - 1) + '"');
        c && (d += ' jstid="' + a.ta + '"');
        return d + (b ? "/>" : ">")
    }
    $G.prototype.apply = function(a) {
        var b;
        b = a.nodeName;
        b = "input" == b || "INPUT" == b || "option" == b || "OPTION" == b || "select" == b || "SELECT" == b || "textarea" == b || "TEXTAREA" == b;
        this.K = !1;
        var c;
        a: {
            c = null == this.A ? 0 : this.A.length;
            var d = this.B == c;d ? this.C = this.A : -1 != this.B && cH(this);
            if (d) {
                if (b)
                    for (d = 0; d < c; d += 7) {
                        var e = this.A[d + 1];
                        if (("checked" == e || "value" == e) && this.A[d + 5] != a[e]) {
                            c = !1;
                            break a
                        }
                    }
                c = !0
            } else c = !1
        }
        if (!c) {
            c = null;
            if (null != this.C && (d = c = {}, 0 != (this.D & 768) && null != this.C))
                for (var e = this.C.length, f = 0; f < e; f += 7)
                    if (null !=
                        this.C[f + 5]) {
                        var g = this.C[f + 0],
                            k = this.C[f + 1],
                            l = this.C[f + 2];
                        5 == g || 7 == g ? d[k + "." + l] = !0 : -1 != g && 18 != g && 20 != g && (d[k] = !0)
                    }
            var m = "",
                e = d = "",
                f = null,
                g = !1,
                n = null;
            a.hasAttribute("class") && (n = a.getAttribute("class").split(" "));
            for (var k = 0 != (this.D & 832) ? "" : null, l = "", p = this.A, r = p ? p.length : 0, u = 0; u < r; u += 7) {
                var t = p[u + 5],
                    v = p[u + 0],
                    x = p[u + 1],
                    D = p[u + 2],
                    z = p[u + 3],
                    A = p[u + 6];
                if (null !== t && null != k && !A) switch (v) {
                    case -1:
                        k += t + ",";
                        break;
                    case 7:
                    case 5:
                        k += v + "." + D + ",";
                        break;
                    case 13:
                        k += v + "." + x + "." + D + ",";
                        break;
                    case 18:
                    case 20:
                        break;
                    default:
                        k +=
                            v + "." + x + ","
                }
                if (!(u < this.I)) switch (null != c && void 0 !== t && (5 == v || 7 == v ? delete c[x + "." + D] : delete c[x]), v) {
                    case 7:
                        null === t ? null != n && tb(n, D) : null != t && (null == n ? n = [D] : sb(n, D) || n.push(D));
                        break;
                    case 4:
                        null === t ? a.style.cssText = "" : void 0 !== t && (a.style.cssText = oH(z, t));
                        for (var B in c) 0 == B.lastIndexOf("style.", 0) && delete c[B];
                        break;
                    case 5:
                        try {
                            B = D.replace(/-(\S)/g, lH), a.style[B] != t && (a.style[B] = t || "")
                        } catch (Q) {}
                        break;
                    case 8:
                        null == f && (f = {});
                        f[x] = null === t ? null : t ? [t, null, z] : [a[x] || a.getAttribute(x) || "", null, z];
                        break;
                    case 18:
                        null != t && ("jsl" == x ? m += t : "jsvs" == x && (e += t));
                        break;
                    case 22:
                        null === t ? a.removeAttribute("jsaction") : null != t && ((v = p[u + 4]) && (t = Va(t)), l && (l += ";"), l += t);
                        break;
                    case 20:
                        null != t && (d && (d += ","), d += t);
                        break;
                    case 21:
                    case 0:
                        null === t ? a.removeAttribute(x) : null != t && ((v = p[u + 4]) && (t = Va(t)), t = oH(z, t), v = a.nodeName, !("CANVAS" != v && "canvas" != v || "width" != x && "height" != x) && t == a.getAttribute(x) || a.setAttribute(x, t));
                        if (b)
                            if ("checked" == x) g = !0;
                            else if (v = x, v = v.toLowerCase(), "value" == v || "checked" == v || "selected" == v || "selectedindex" ==
                            v) v = WG.hasOwnProperty(x) ? WG[x] : x, a[v] != t && (a[v] = t);
                        break;
                    case 14:
                    case 11:
                    case 12:
                    case 10:
                    case 9:
                    case 13:
                        null == f && (f = {}), z = f[x], null !== z && (z || (z = f[x] = [a[x] || a.getAttribute(x) || "", null, null]), ZG(z, v, D, t))
                }
            }
            if (null != c)
                for (B in c)
                    if (0 == B.lastIndexOf("class.", 0)) tb(n, B.substr(6));
                    else if (0 == B.lastIndexOf("style.", 0)) try {
                a.style[B.substr(6).replace(/-(\S)/g, lH)] = ""
            } catch (Q) {} else 0 != (this.D & 512) && "data-rtid" != B && a.removeAttribute(B);
            null != n && 0 < n.length ? a.setAttribute("class", sG(n.join(" "))) : a.hasAttribute("class") &&
                a.setAttribute("class", "");
            if (null != m && "" != m && a.hasAttribute("jsl")) {
                B = a.getAttribute("jsl");
                b = m.charAt(0);
                for (c = 0;;) {
                    c = B.indexOf(b, c);
                    if (-1 == c) {
                        m = B + m;
                        break
                    }
                    if (0 == m.lastIndexOf(B.substr(c), 0)) {
                        m = B.substr(0, c) + m;
                        break
                    }
                    c += 1
                }
                a.setAttribute("jsl", m)
            }
            if (null != f)
                for (x in f) z = f[x], null === z ? (a.removeAttribute(x), a[x] = null) : (B = nH(this, x, z), a[x] = B, a.setAttribute(x, B));
            l && a.setAttribute("jsaction", l);
            d && a.setAttribute("jsinstance", d);
            e && a.setAttribute("jsvs", e);
            null != k && (-1 != k.indexOf(".") ? a.setAttribute("jsan",
                k.substr(0, k.length - 1)) : a.removeAttribute("jsan"));
            g && (a.checked = !!a.getAttribute("checked"))
        }
    };

    function oH(a, b) {
        switch (a) {
            case null:
                return b;
            case 2:
                return CG(b);
            case 1:
                return a = Gi(b).lb(), "about:invalid#zClosurez" === a ? "about:invalid#zjslayoutz" : a;
            case 8:
                return EG(b);
            default:
                return "sanitization_error_" + a
        }
    };
    var qH = {};

    function rH() {
        this.D = this.qa = this.C = this.B = this.error = this.A = null
    };

    function sH(a, b) {
        this.B = a;
        this.A = b
    }
    sH.prototype.get = function(a) {
        return this.B.A[this.A[a]] || null
    };
    var tH = /\s*;\s*/,
        uH = /&/g,
        vH = /^[$a-z_]*$/i,
        wH = /^[\$_a-z][\$_0-9a-z]*$/i,
        xH = /^\s*$/,
        yH = /^((de|en)codeURI(Component)?|is(Finite|NaN)|parse(Float|Int)|document|false|function|jslayout|null|this|true|undefined|window|Array|Boolean|Date|Error|JSON|Math|Number|Object|RegExp|String|__event)$/,
        zH = /[\$_a-z][\$_0-9a-z]*|'(\\\\|\\'|\\?[^'\\])*'|"(\\\\|\\"|\\?[^"\\])*"|[0-9]*\.?[0-9]+([e][-+]?[0-9]+)?|0x[0-9a-f]+|\-|\+|\*|\/|\%|\=|\<|\>|\&\&?|\|\|?|\!|\^|\~|\(|\)|\{|\}|\[|\]|\,|\;|\.|\?|\:|\@|#[0-9]+|[\s]+/gi,
        AH = {},
        BH = {};

    function CH(a) {
        var b = a.match(zH);
        null == b && (b = []);
        if (b.join("").length != a.length) {
            for (var c = 0, d = 0; d < b.length && a.substr(c, b[d].length) == b[d]; d++) c += b[d].length;
            throw Error("Parsing error at position " + c + " of " + a);
        }
        return b
    }

    function DH(a, b, c) {
        for (var d = !1, e = []; b < c; b++) {
            var f = a[b];
            if ("{" == f) d = !0, e.push("}");
            else if ("." == f || "new" == f || "," == f && "}" == e[e.length - 1]) d = !0;
            else if (xH.test(f)) a[b] = " ";
            else {
                if (!d && wH.test(f) && !yH.test(f)) {
                    if (a[b] = (null != AF[f] ? "g" : "v") + "." + f, "has" == f || "size" == f) b = EH(a, b + 1)
                } else if ("(" == f) e.push(")");
                else if ("[" == f) e.push("]");
                else if (")" == f || "]" == f || "}" == f) {
                    if (0 == e.length) throw Error('Unexpected "' + f + '".');
                    d = e.pop();
                    if (f != d) throw Error('Expected "' + d + '" but found "' + f + '".');
                }
                d = !1
            }
        }
        if (0 != e.length) throw Error("Missing bracket(s): " +
            e.join());
    }

    function EH(a, b) {
        for (;
            "(" != a[b] && b < a.length;) b++;
        a[b] = "(function(){return ";
        if (b == a.length) throw Error('"(" missing for has() or size().');
        b++;
        for (var c = b, d = 0, e = !0; b < a.length;) {
            var f = a[b];
            if ("(" == f) d++;
            else if (")" == f) {
                if (0 == d) break;
                d--
            } else "" != f.trim() && '"' != f.charAt(0) && "'" != f.charAt(0) && "+" != f && (e = !1);
            b++
        }
        if (b == a.length) throw Error('matching ")" missing for has() or size().');
        a[b] = "})";
        d = a.slice(c, b).join("").trim();
        if (e)
            for (e = "" + eval(d), e = CH(e), DH(e, 0, e.length), a[c] = e.join(""), c += 1; c < b; c++) a[c] =
                "";
        else DH(a, c, b);
        return b
    }

    function FH(a, b) {
        for (var c = a.length; b < c; b++) {
            var d = a[b];
            if (":" == d) return b;
            if ("{" == d || "?" == d || ";" == d) break
        }
        return -1
    }

    function GH(a, b) {
        for (var c = a.length; b < c; b++)
            if (";" == a[b]) return b;
        return c
    }

    function HH(a) {
        a = CH(a);
        return IH(a)
    }

    function JH(a) {
        return new Function("scope", "value", 'scope["' + a + '"] = value;')
    }

    function IH(a, b) {
        DH(a, 0, a.length);
        a = a.join("");
        b && (a = 'v["' + b + '"] = ' + a);
        b = BH[a];
        b || (b = new Function("v", "g", "return " + a), BH[a] = b);
        return b
    }

    function KH(a) {
        return a
    }
    var LH = [];

    function MH(a) {
        LH.length = 0;
        for (var b = 5; b < a.length; ++b) {
            var c = a[b];
            uH.test(c) ? LH.push(c.replace(uH, "&&")) : LH.push(c)
        }
        return LH.join("&")
    }

    function NH(a) {
        var b = [],
            c;
        for (c in AH) delete AH[c];
        a = CH(a);
        c = 0;
        for (var d = a.length; c < d;) {
            for (var e = [null, null, null, null, null], f = "", g = ""; c < d; c++) {
                g = a[c];
                if ("?" == g || ":" == g) {
                    "" != f && e.push(f);
                    break
                }
                xH.test(g) || ("." == g ? ("" != f && e.push(f), f = "") : f = '"' == g.charAt(0) || "'" == g.charAt(0) ? f + eval(g) : f + g)
            }
            if (c >= d) break;
            var f = GH(a, c + 1),
                k = MH(e),
                l = AH[k],
                m = "undefined" == typeof l;
            m && (l = AH[k] = b.length, b.push(e));
            e = b[l];
            e[1] = BG(e);
            c = IH(a.slice(c + 1, f));
            ":" == g ? e[4] = c : "?" == g && (e[3] = c);
            if (m) {
                var n, g = e[5];
                "class" == g || "className" ==
                    g ? 6 == e.length ? n = 6 : (e.splice(5, 1), n = 7) : "style" == g ? 6 == e.length ? n = 4 : (e.splice(5, 1), n = 5) : g in PG ? 6 == e.length ? n = 8 : "hash" == e[6] ? (n = 14, e.length = 6) : "host" == e[6] ? (n = 11, e.length = 6) : "path" == e[6] ? (n = 12, e.length = 6) : "param" == e[6] && 8 <= e.length ? (n = 13, e.splice(6, 1)) : "port" == e[6] ? (n = 10, e.length = 6) : "protocol" == e[6] ? (n = 9, e.length = 6) : b.splice(l, 1) : n = 0;
                e[0] = n
            }
            c = f + 1
        }
        return b
    }

    function OH(a, b) {
        var c = JH(a);
        return function(a) {
            var d = b(a);
            c(a, d);
            return d
        }
    };
    var PH = 0,
        QH = {
            0: []
        },
        RH = {};

    function SH(a, b) {
        var c = String(++PH);
        RH[b] = c;
        QH[c] = a;
        return c
    }

    function TH(a, b) {
        a.setAttribute("jstcache", b);
        a.__jstcache = QH[b]
    }
    var UH = [];

    function VH(a) {
        a.length = 0;
        UH.push(a)
    }
    for (var WH = [
            ["jscase", HH, "$sc"],
            ["jscasedefault", KH, "$sd"],
            ["jsl", null, null],
            ["jsglobals", function(a) {
                var b = [];
                a = a.split(tH);
                for (var c = 0, d = a ? a.length : 0; c < d; ++c) {
                    var e = La(a[c]);
                    if (e) {
                        var f = e.indexOf(":");
                        if (-1 != f) {
                            var g = La(e.substring(0, f)),
                                e = La(e.substring(f + 1)),
                                f = e.indexOf(" "); - 1 != f && (e = e.substring(f + 1));
                            b.push([JH(g), e])
                        }
                    }
                }
                return b
            }, "$g", !0],
            ["jsfor", function(a) {
                var b = [];
                a = CH(a);
                for (var c = 0, d = a.length; c < d;) {
                    var e = [],
                        f = FH(a, c);
                    if (-1 == f) {
                        if (xH.test(a.slice(c, d).join(""))) break;
                        f = c - 1
                    } else
                        for (var g =
                                c; g < f;) {
                            var k = mb(a, ",", g);
                            if (-1 == k || k > f) k = f;
                            e.push(JH(La(a.slice(g, k).join(""))));
                            g = k + 1
                        }
                    0 == e.length && e.push(JH("$this"));
                    1 == e.length && e.push(JH("$index"));
                    2 == e.length && e.push(JH("$count"));
                    if (3 != e.length) throw Error("Max 3 vars for jsfor; got " + e.length);
                    c = GH(a, c);
                    e.push(IH(a.slice(f + 1, c)));
                    b.push(e);
                    c += 1
                }
                return b
            }, "for", !0],
            ["jskey", HH, "$k"],
            ["jsdisplay", HH, "display"],
            ["jsmatch", null, null],
            ["jsif", HH, "display"],
            [null, HH, "$if"],
            ["jsvars", function(a) {
                var b = [];
                a = CH(a);
                for (var c = 0, d = a.length; c < d;) {
                    var e =
                        FH(a, c);
                    if (-1 == e) break;
                    var f = GH(a, e + 1),
                        c = La(a.slice(c, e).join("")),
                        e = IH(a.slice(e + 1, f), c);
                    b.push(e);
                    c = f + 1
                }
                return b
            }, "var", !0],
            [null, function(a) {
                return [JH(a)]
            }, "$vs"],
            ["jsattrs", NH, "_a", !0],
            [null, NH, "$a", !0],
            [null, function(a) {
                var b = a.indexOf(":");
                return [a.substr(0, b), a.substr(b + 1)]
            }, "$ua"],
            [null, function(a) {
                var b = a.indexOf(":");
                return [a.substr(0, b), HH(a.substr(b + 1))]
            }, "$uae"],
            [null, function(a) {
                var b = [];
                a = CH(a);
                for (var c = 0, d = a.length; c < d;) {
                    var e = FH(a, c);
                    if (-1 == e) break;
                    var f = GH(a, e + 1),
                        c = La(a.slice(c,
                            e).join("")),
                        e = IH(a.slice(e + 1, f), c);
                    b.push([c, e]);
                    c = f + 1
                }
                return b
            }, "$ia", !0],
            [null, function(a) {
                var b = [];
                a = CH(a);
                for (var c = 0, d = a.length; c < d;) {
                    var e = FH(a, c);
                    if (-1 == e) break;
                    var f = GH(a, e + 1),
                        c = La(a.slice(c, e).join("")),
                        e = IH(a.slice(e + 1, f), c);
                    b.push([c, JH(c), e]);
                    c = f + 1
                }
                return b
            }, "$ic", !0],
            [null, KH, "$rj"],
            ["jseval", function(a) {
                var b = [];
                a = CH(a);
                for (var c = 0, d = a.length; c < d;) {
                    var e = GH(a, c);
                    b.push(IH(a.slice(c, e)));
                    c = e + 1
                }
                return b
            }, "$e", !0],
            ["jsskip", HH, "$sk"],
            ["jsswitch", HH, "$s"],
            ["jscontent", function(a) {
                var b =
                    a.indexOf(":"),
                    c = null;
                if (-1 != b) {
                    var d = La(a.substr(0, b));
                    vH.test(d) && (c = "html_snippet" == d ? 1 : "raw" == d ? 2 : "safe" == d ? 7 : null, a = La(a.substr(b + 1)))
                }
                return [c, !1, HH(a)]
            }, "$c"],
            ["transclude", KH, "$u"],
            [null, HH, "$ue"],
            [null, null, "$up"]
        ], XH = {}, YH = 0; YH < WH.length; ++YH) {
        var ZH = WH[YH];
        ZH[2] && (XH[ZH[2]] = [ZH[1], ZH[3]])
    }
    XH.$t = [KH, !1];
    XH.$x = [KH, !1];
    XH.$u = [KH, !1];

    function $H(a, b) {
        if (!b || !b.getAttribute) return null;
        aI(a, b, null);
        var c = b.__rt;
        return c && c.length ? c[c.length - 1] : $H(a, b.parentNode)
    }

    function bI(a) {
        var b = QH[RH[a + " 0"] || "0"];
        "$t" != b[0] && (b = ["$t", a].concat(b));
        return b
    }
    var cI = /^\$x (\d+);?/;

    function dI(a, b) {
        a = RH[b + " " + a];
        return QH[a] ? a : null
    }

    function eI(a, b) {
        a = dI(a, b);
        return null != a ? QH[a] : null
    }

    function fI(a, b, c, d, e) {
        if (d == e) return VH(b), "0";
        "$t" == b[0] ? a = b[1] + " 0" : (a += ":", a = 0 == d && e == c.length ? a + c.join(":") : a + c.slice(d, e).join(":"));
        (c = RH[a]) ? VH(b): c = SH(b, a);
        return c
    }
    var gI = /\$t ([^;]*)/g;

    function hI(a) {
        var b = a.__rt;
        b || (b = a.__rt = []);
        return b
    }

    function aI(a, b, c) {
        if (!b.__jstcache) {
            b.hasAttribute("jstid") && (b.getAttribute("jstid"), b.removeAttribute("jstid"));
            var d = b.getAttribute("jstcache");
            if (null != d && QH[d]) b.__jstcache = QH[d];
            else {
                d = b.getAttribute("jsl");
                gI.lastIndex = 0;
                for (var e; e = gI.exec(d);) hI(b).push(e[1]);
                null == c && (c = String($H(a, b.parentNode)));
                if (a = cI.exec(d)) e = a[1], d = dI(e, c), null == d && (a = UH.length ? UH.pop() : [], a.push("$x"), a.push(e), e = c + ":" + a.join(":"), (d = RH[e]) && QH[d] ? VH(a) : d = SH(a, e)), TH(b, d), b.removeAttribute("jsl");
                else {
                    a = UH.length ?
                        UH.pop() : [];
                    d = 0;
                    for (e = WH.length; d < e; ++d) {
                        var f = WH[d],
                            g = f[0];
                        if (g) {
                            var k = b.getAttribute(g);
                            if (k) {
                                f = f[2];
                                if ("jsl" == g) {
                                    for (var f = k, k = a, l = CH(f), m = l.length, n = 0, p = ""; n < m;) {
                                        var r = GH(l, n);
                                        xH.test(l[n]) && n++;
                                        if (!(n >= r)) {
                                            var u = l[n++];
                                            if (!wH.test(u)) throw Error('Cmd name expected; got "' + u + '" in "' + f + '".');
                                            if (n < r && !xH.test(l[n])) throw Error('" " expected between cmd and param.');
                                            n = l.slice(n + 1, r).join("");
                                            "$a" == u ? p += n + ";" : (p && (k.push("$a"), k.push(p), p = ""), XH[u] && (k.push(u), k.push(n)))
                                        }
                                        n = r + 1
                                    }
                                    p && (k.push("$a"),
                                        k.push(p))
                                } else if ("jsmatch" == g)
                                    for (f = a, k = CH(k), l = k.length, r = 0; r < l;) m = FH(k, r), p = GH(k, r), r = k.slice(r, p).join(""), xH.test(r) || (-1 !== m ? (f.push("display"), f.push(k.slice(m + 1, p).join("")), f.push("var")) : f.push("display"), f.push(r)), r = p + 1;
                                else a.push(f), a.push(k);
                                b.removeAttribute(g)
                            }
                        }
                    }
                    if (0 == a.length) TH(b, "0");
                    else {
                        if ("$u" == a[0] || "$t" == a[0]) c = a[1];
                        e = c + ":" + a.join(":");
                        d = RH[e];
                        if (!d || !QH[d]) a: {
                            d = a;e = "0";g = UH.length ? UH.pop() : [];k = f = 0;
                            for (l = d.length; k < l; k += 2) {
                                m = d[k];
                                r = d[k + 1];
                                p = XH[m];
                                u = p[1];
                                p = (0, p[0])(r);
                                "$t" ==
                                m && r && (c = r);
                                if ("$k" == m) "for" == g[g.length - 2] && (g[g.length - 2] = "$fk", g[g.length - 2 + 1].push(p));
                                else if ("$t" == m && "$x" == d[k + 2]) {
                                    p = dI("0", c);
                                    if (null != p) {
                                        0 == f && (e = p);
                                        VH(g);
                                        d = e;
                                        break a
                                    }
                                    g.push("$t");
                                    g.push(r)
                                } else if (u)
                                    for (r = 0, u = p.length; r < u; ++r)
                                        if (n = p[r], "_a" == m) {
                                            var t = n[0],
                                                v = n[5],
                                                x = v.charAt(0);
                                            "$" == x ? (g.push("var"), g.push(OH(n[5], n[4]))) : "@" == x ? (g.push("$a"), n[5] = v.substr(1), g.push(n)) : 6 == t || 7 == t || 4 == t || 5 == t || "jsaction" == v || "jsnamespace" == v || v in PG ? (g.push("$a"), g.push(n)) : (WG.hasOwnProperty(v) && (n[5] = WG[v]),
                                                6 == n.length && (g.push("$a"), g.push(n)))
                                        } else g.push(m), g.push(n);
                                else g.push(m), g.push(p);
                                if ("$u" == m || "$ue" == m || "$up" == m || "$x" == m) m = k + 2, p = fI(c, g, d, f, m), 0 == f && (e = p), g = [], f = m
                            }
                            p = fI(c, g, d, f, d.length);0 == f && (e = p);d = e
                        }
                        TH(b, d)
                    }
                    VH(a)
                }
            }
        }
    }

    function iI(a) {
        return function() {
            return a
        }
    };

    function jI(a) {
        var b = pa("google.cd");
        b && a(b)
    }

    function kI(a, b) {
        jI(function(c) {
            c.c(a, null, void 0, void 0, b)
        })
    };

    function lI(a) {
        a = a.split("$");
        this.B = a[0];
        this.A = a[1] || null
    }

    function mI(a, b, c) {
        var d = b.call(c, a.B);
        y(d) || null == a.A || (d = b.call(c, a.A));
        return d
    };

    function nI(a) {
        this.C = a;
        this.A = {};
        this.F = {};
        this.H = {};
        this.G = {};
        this.D = {};
        this.B = C
    }

    function oI(a, b) {
        return !!mI(new lI(b), function(a) {
            return this.A[a]
        }, a)
    }
    nI.prototype.Oa = function() {
        for (var a in this.A)
            if (this.A.hasOwnProperty(a)) return !1;
        return !0
    };

    function pI(a, b, c, d) {
        b = mI(new lI(b), function(a) {
            return a in this.A ? a : void 0
        }, a);
        var e = a.F[b],
            f = a.H[b],
            g = a.G[b],
            k = a.D[b];
        try {
            var l = new e;
            c.A = l;
            l.nh = c;
            l.Ld = b;
            c.B = a;
            var m = f ? new f(d) : null;
            c.C = m;
            var n = g ? new g(l) : null;
            c.qa = n;
            a.B("controller_init", l.Ld);
            k(l, m, n);
            a.B("controller_init", l.Ld);
            return l
        } catch (p) {
            c.A = null;
            c.error = p;
            kI(b, p);
            try {
                a.C.A(p)
            } catch (r) {}
            return null
        }
    };

    function qI() {
        this.A = C
    };

    function rI() {
        this.A = {}
    }
    rI.prototype.add = function(a, b) {
        this.A[a] = b
    };

    function sI(a, b) {
        this.B = y(a) ? a : document;
        this.G = null;
        this.H = {};
        this.C = [];
        this.F = b || new rI;
        this.K = this.B ? ob(this.B.getElementsByTagName("style"), function(a) {
            return a.innerHTML
        }).join() : "";
        this.A = {}
    }
    sI.prototype.document = h("B");

    function tI(a) {
        var b = a.B.createElement("STYLE");
        a.B.head ? a.B.head.appendChild(b) : a.B.body.appendChild(b);
        return b
    };

    function uI(a, b, c) {
        sI.call(this, a, c);
        this.D = b || new nI(new qI);
        this.I = []
    }
    H(uI, sI);
    var vI = [];

    function wI(a, b) {
        if ("number" == typeof a[3]) {
            var c = a[3];
            a[3] = b[c];
            a.lc = c
        } else "undefined" == typeof a[3] && (a[3] = vI, a.lc = -1);
        "number" != typeof a[1] && (a[1] = 0);
        if ((a = a[4]) && "string" != typeof a)
            for (c = 0; c < a.length; ++c) a[c] && "string" != typeof a[c] && wI(a[c], b)
    };
    var xI = ["unresolved", null];

    function yI(a) {
        this.element = a;
        this.C = this.F = this.B = this.A = this.next = null;
        this.D = !1
    }

    function zI() {
        this.B = null;
        this.D = String;
        this.C = "";
        this.A = null
    }

    function AI(a, b, c, d, e) {
        this.B = a;
        this.F = b;
        this.M = this.I = this.H = 0;
        this.O = "";
        this.J = [];
        this.L = !1;
        this.T = c;
        this.A = d;
        this.K = 0;
        this.G = this.C = null;
        this.D = e;
        this.N = null
    }

    function BI(a, b) {
        return a == b || null != a.G && BI(a.G, b) ? !0 : 2 == a.K && null != a.C && null != a.C[0] && BI(a.C[0], b)
    }

    function CI(a, b, c) {
        if (a.B == xI && a.D == b) return a;
        if (null != a.J && 0 < a.J.length && "$t" == a.B[a.H]) {
            if (a.B[a.H + 1] == b) return a;
            c && c.push(a.B[a.H + 1])
        }
        if (null != a.G) {
            var d = CI(a.G, b, c);
            if (d) return d
        }
        return 2 == a.K && null != a.C && null != a.C[0] ? CI(a.C[0], b, c) : null
    }

    function DI(a) {
        var b = a.N;
        if (null != b) {
            var c = b["action:load"];
            null != c && (c.call(a.T.element), b["action:load"] = null);
            c = b["action:create"];
            null != c && (c.call(a.T.element), b["action:create"] = null)
        }
        null != a.G && DI(a.G);
        2 == a.K && null != a.C && null != a.C[0] && DI(a.C[0])
    };

    function EI(a, b, c) {
        this.B = a;
        this.G = a.document();
        ++DF;
        this.F = this.D = this.A = null;
        this.C = !1;
        this.I = 2 == (b & 2);
        this.H = null == c ? null : F() + c
    }
    var FI = [];

    function GI(a, b, c) {
        if (a.D == b) b = null;
        else if (a.D == c) return null == b;
        if (null != a.G) return GI(a.G, b, c);
        if (null != a.C)
            for (var d = 0; d < a.C.length; d++) {
                var e = a.C[d];
                if (null != e) {
                    if (e.T.element != a.T.element) break;
                    e = GI(e, b, c);
                    if (null != e) return e
                }
            }
        return null
    }

    function HI(a, b, c, d) {
        if (c != a) return ck(a, c);
        if (b == d) return !0;
        a = a.__cdn;
        return null != a && 1 == GI(a, b, d)
    }

    function II(a, b) {
        if (b.T.element && !b.T.element.__cdn) JI(a, b);
        else if (KI(b)) {
            var c = b.D;
            if (b.T.element) {
                var d = b.T.element;
                if (b.L) {
                    var e = b.T.A;
                    null != e && e.reset(c || void 0)
                }
                for (var c = b.J, e = !!b.A.A.Fa, f = c.length, g = 1 == b.K, k = b.H, l = 0; l < f; ++l) {
                    var m = c[l],
                        n = b.B[k],
                        p = LI[n];
                    if (null != m)
                        if (null == m.B) p.method.call(a, b, m, k);
                        else {
                            var r = GF(b.A, m.B, d),
                                u = m.D(r);
                            if (0 != p.la) {
                                if (p.method.call(a, b, m, k, r, m.C != u), m.C = u, ("display" == n || "$if" == n) && !r || "$sk" == n && r) {
                                    g = !1;
                                    break
                                }
                            } else u != m.C && (m.C = u, p.method.call(a, b, m, k, r))
                        }
                    k += 2
                }
                g &&
                    (MI(a, b.T, b), NI(a, b));
                b.A.A.Fa = e
            } else NI(a, b)
        }
    }

    function NI(a, b) {
        if (1 == b.K && (b = b.C, null != b))
            for (var c = 0; c < b.length; ++c) {
                var d = b[c];
                null != d && II(a, d)
            }
    }

    function OI(a, b) {
        var c = a.__cdn;
        null != c && BI(c, b) || (a.__cdn = b)
    }

    function JI(a, b) {
        var c = b.T.element;
        if (!KI(b)) return !1;
        var d = b.D;
        c.__vs && (c.__vs[0] = 1);
        OI(c, b);
        c = !!b.A.A.Fa;
        if (!b.B.length) return b.C = [], b.K = 1, PI(a, b, d), b.A.A.Fa = c, !0;
        b.L = !0;
        QI(a, b);
        b.A.A.Fa = c;
        return !0
    }

    function PI(a, b, c) {
        for (var d = b.A, e = Zj(b.T.element); e; e = bk(e)) {
            var f = new AI(RI(a, e, c), null, new yI(e), d, c);
            JI(a, f);
            e = f.T.next || f.T.element;
            0 == f.J.length && e.__cdn ? null != f.C && xb(b.C, f.C) : b.C.push(f)
        }
    }

    function SI(a, b, c) {
        var d = b.A,
            e = b.F[4];
        if (e)
            if ("string" == typeof e) a.A += e;
            else
                for (var f = !!d.A.Fa, g = 0; g < e.length; ++g) {
                    var k = e[g];
                    if ("string" == typeof k) a.A += k;
                    else {
                        var k = new AI(k[3], k, new yI(null), d, c),
                            l = a,
                            m = k;
                        if (0 == m.B.length) {
                            var n = m.D,
                                p = m.T;
                            m.C = [];
                            m.K = 1;
                            TI(l, m);
                            MI(l, p, m);
                            if (0 != (p.A.D & 2048)) {
                                var r = m.A.A.Ya;
                                m.A.A.Ya = !1;
                                SI(l, m, n);
                                m.A.A.Ya = !1 !== r
                            } else SI(l, m, n);
                            UI(l, p, m)
                        } else m.L = !0, QI(l, m);
                        0 != k.J.length ? b.C.push(k) : null != k.C && xb(b.C, k.C);
                        d.A.Fa = f
                    }
                }
    }

    function VI(a, b, c) {
        var d = b.T;
        d.D = !0;
        !1 === b.A.A.Ya ? (MI(a, d, b), UI(a, d, b)) : (d = a.C, a.C = !0, QI(a, b, c), a.C = d)
    }

    function QI(a, b, c) {
        var d = b.T,
            e = b.D,
            f = b.B,
            g = c || b.H;
        if (0 == g)
            if ("$t" == f[0] && "$x" == f[2]) {
                var k = f[3];
                c = f[1];
                k = eI(k, c);
                if (null != k) {
                    b.B = k;
                    b.D = c;
                    QI(a, b);
                    return
                }
            } else if ("$x" == f[0] && (k = f[1], k = eI(k, e), null != k)) {
            b.B = k;
            QI(a, b);
            return
        }
        for (c = f.length; g < c; g += 2) {
            var k = f[g],
                l = f[g + 1];
            "$t" == k && (e = l);
            d.A || (null != a.A ? "for" != k && "$fk" != k && TI(a, b) : "$a" != k && "$u" != k && "$ua" != k && "$uae" != k && "$ue" != k && "$up" != k && "display" != k && "$if" != k && "$dd" != k && "$dc" != k && "$dh" != k && "$sk" != k || WI(d, e));
            if (k = LI[k]) {
                var m = new zI,
                    l = b,
                    n = m,
                    p = l.B[g + 1];
                switch (l.B[g]) {
                    case "$ue":
                        n.D = JF;
                        n.B = p;
                        break;
                    case "for":
                        n.D = XI;
                        n.B = p[3];
                        break;
                    case "$fk":
                        n.A = [];
                        n.D = YI(l.A, l.T, p, n.A);
                        n.B = p[3];
                        break;
                    case "display":
                    case "$if":
                    case "$sk":
                    case "$s":
                        n.B = p;
                        break;
                    case "$c":
                        n.B = p[2]
                }
                var l = a,
                    n = b,
                    p = g,
                    r = n.T,
                    u = r.element,
                    t = n.B[p],
                    v = n.A,
                    x = null;
                if (m.B)
                    if (l.C) {
                        x = "";
                        switch (t) {
                            case "$ue":
                                x = ZI;
                                break;
                            case "for":
                            case "$fk":
                                x = FI;
                                break;
                            case "display":
                            case "$if":
                            case "$sk":
                                x = !0;
                                break;
                            case "$s":
                                x = 0;
                                break;
                            case "$c":
                                x = ""
                        }
                        x = $I(v, m.B, u, x)
                    } else x = GF(v, m.B, u);
                u = m.D(x);
                m.C = u;
                t = LI[t];
                4 == t.la ?
                    (n.C = [], n.K = t.A) : 3 == t.la && (r = n.G = new AI(xI, null, r, new BF, "null"), r.I = n.I + 1, r.M = n.M);
                n.J.push(m);
                t.method.call(l, n, m, p, x, !0);
                if (0 != k.la) return
            } else g == b.H ? b.H += 2 : b.J.push(null)
        }
        if (null == a.A || "style" != d.A.name()) MI(a, d, b), b.C = [], b.K = 1, null != a.A ? SI(a, b, e) : PI(a, b, e), 0 == b.C.length && (b.C = null), UI(a, d, b)
    }

    function $I(a, b, c, d) {
        try {
            return GF(a, b, c)
        } catch (e) {
            return d
        }
    }
    var ZI = new IF("null");

    function XI(a) {
        return String(aJ(a).length)
    }
    EI.prototype.K = function(a, b, c, d, e) {
        MI(this, a.T, a);
        c = a.C;
        if (e)
            if (null != this.A) {
                c = a.C;
                e = a.A;
                for (var f = a.F[4], g = -1, k = 0; k < f.length; ++k) {
                    var l = f[k][3];
                    if ("$sc" == l[0]) {
                        if (GF(e, l[1], null) === d) {
                            g = k;
                            break
                        }
                    } else "$sd" == l[0] && (g = k)
                }
                b.A = g;
                for (k = 0; k < f.length; ++k) b = f[k], b = c[k] = new AI(b[3], b, new yI(null), e, a.D), this.C && (b.T.D = !0), k == g ? QI(this, b) : a.F[2] && VI(this, b);
                UI(this, a.T, a)
            } else {
                e = a.A;
                k = a.T.element;
                g = [];
                f = -1;
                for (k = Zj(k); k; k = bk(k)) l = RI(this, k, a.D), "$sc" == l[0] ? (g.push(k), GF(e, l[1], k) === d && (f = g.length - 1)) : "$sd" ==
                    l[0] && (g.push(k), -1 == f && (f = g.length - 1)), k = UG(k);
                d = 0;
                for (l = g.length; d < l; ++d) {
                    var m = d == f,
                        k = c[d];
                    m || null == k || bJ(this.B, k, !0);
                    for (var k = g[d], n = UG(k), p = !0; p; k = k.nextSibling) ao(k, m), k == n && (p = !1)
                }
                b.A = f; - 1 != f && (b = c[f], null == b ? (b = g[f], k = c[f] = new AI(RI(this, b, a.D), null, new yI(b), e, a.D), JI(this, k)) : II(this, b))
            }
        else -1 != b.A && (f = b.A, II(this, c[f]))
    };

    function cJ(a, b) {
        a = a.B;
        for (var c in a) b.A[c] = a[c]
    }

    function dJ(a, b) {
        this.B = a;
        this.A = b;
        this.ac = null
    }
    dJ.prototype.ra = function() {
        if (null != this.ac)
            for (var a = 0; a < this.ac.length; ++a) this.ac[a].B(this)
    };

    function eJ(a) {
        null == a.N && (a.N = {});
        return a.N
    }
    q = EI.prototype;
    q.Mk = function(a, b, c) {
        b = a.A;
        var d = a.T.element;
        c = a.B[c + 1];
        var e = c[0],
            f = c[1];
        c = eJ(a);
        var e = "observer:" + e,
            g = c[e];
        b = GF(b, f, d);
        if (null != g) {
            if (g.ac[0] == b) return;
            g.ra()
        }
        a = new dJ(this.B, a);
        null == a.ac ? a.ac = [b] : a.ac.push(b);
        b.A(a);
        c[e] = a
    };
    q.Ym = function(a, b, c, d, e) {
        c = a.G;
        e && (c.J.length = 0, c.D = d.A, c.B = xI);
        if (!fJ(this, a, b)) {
            e = a.T;
            var f = this.B.A[d.A];
            null != f && (fH(e.A, 768), HF(c.A, a.A, FI), cJ(d, c.A), gJ(this, a, c, f, b, d.B))
        }
    };
    q.Hb = function(a, b, c) {
        if (null != this.A) return !1;
        if (null != this.H && this.H <= F()) {
            a: {
                c = new dJ(this.B, a);
                var d = c.A.T.element;b = c.A.D;a = c.B.I;
                if (0 != a.length)
                    for (var e = a.length - 1; 0 <= e; --e) {
                        var f = a[e],
                            g = f.A.T.element,
                            f = f.A.D;
                        if (HI(g, f, d, b)) break a;
                        HI(d, b, g, f) && a.splice(e, 1)
                    }
                a.push(c)
            }
            return !0
        }
        e = b.A;
        if (null == e) b.A = e = new BF, HF(e, a.A), c = !0;
        else {
            b = e;
            a = a.A;
            e = !1;
            for (d in b.A)
                if (g = a.A[d], b.A[d] != g && (b.A[d] = g, c && ua(c) ? -1 != c.indexOf(d) : null != c[d])) e = !0;
            c = e
        }
        return this.I && !c
    };

    function hJ(a, b, c) {
        return null != a.A && a.C && b.F[2] ? (c.C = "", !0) : !1
    }

    function fJ(a, b, c) {
        return hJ(a, b, c) ? (MI(a, b.T, b), UI(a, b.T, b), !0) : !1
    }
    q.Vm = function(a, b, c) {
        if (!fJ(this, a, b)) {
            var d = a.G;
            c = a.B[c + 1];
            d.D = c;
            c = this.B.A[c];
            null != c && (HF(d.A, a.A, c.oc), gJ(this, a, d, c, b, c.oc))
        }
    };

    function gJ(a, b, c, d, e, f) {
        if (null == e || null == d || !d.async || !a.Hb(c, e, f))
            if (c.B != xI) II(a, c);
            else {
                f = c.T;
                (e = f.element) && OI(e, c);
                null == f.B && (f.B = e ? hI(e) : []);
                f = f.B;
                var g = c.I;
                f.length < g - 1 ? (c.B = bI(c.D), QI(a, c)) : f.length == g - 1 ? iJ(a, b, c) : f[g - 1] != c.D ? (f.length = g - 1, null != b && bJ(a.B, b, !1), iJ(a, b, c)) : e && (null == d || null == d.Od ? 0 : d.Od != e.getAttribute("jssc")) ? (f.length = g - 1, iJ(a, b, c)) : (c.B = bI(c.D), QI(a, c))
            }
    }
    q.Zm = function(a, b, c) {
        var d = a.B[c + 1];
        if (d[2] || !fJ(this, a, b)) {
            var e = a.G;
            e.D = d[0];
            var f = this.B.A[e.D];
            if (null != f) {
                var g = e.A;
                HF(g, a.A, FI);
                c = a.T.element;
                if (d = d[1])
                    for (var k in d) {
                        var l = GF(a.A, d[k], c);
                        g.A[k] = l
                    }
                f.Bg ? (MI(this, a.T, a), b = f.ek(this.B, g.A), null != this.A ? this.A += b : (OG(c, b), "TEXTAREA" != c.nodeName && "textarea" != c.nodeName || c.value === b || (c.value = b)), UI(this, a.T, a)) : gJ(this, a, e, f, b, d)
            }
        }
    };
    q.Wm = function(a, b, c) {
        var d = a.B[c + 1];
        c = d[0];
        var e = d[1],
            f = a.T,
            g = f.A;
        if (!f.element || "NARROW_PATH" != f.element.__narrow_strategy)
            if (f = this.B.A[e])
                if (d = d[2], null == d || GF(a.A, d, null)) d = b.A, null == d && (b.A = d = new BF), HF(d, a.A, f.oc), "*" == c ? jJ(this, e, f, d, g) : kJ(this, e, f, c, d, g)
    };
    q.Xm = function(a, b, c) {
        var d = a.B[c + 1];
        c = d[0];
        var e = a.T.element;
        if (!e || "NARROW_PATH" != e.__narrow_strategy) {
            var f = a.T.A,
                e = GF(a.A, d[1], e),
                g = e.A,
                k = this.B.A[g];
            k && (d = d[2], null == d || GF(a.A, d, null)) && (d = b.A, null == d && (b.A = d = new BF), HF(d, a.A, FI), cJ(e, d), "*" == c ? jJ(this, g, k, d, f) : kJ(this, g, k, c, d, f))
        }
    };

    function kJ(a, b, c, d, e, f) {
        e.A.Ya = !1;
        var g = "";
        if (c.elements || c.Bg) c.Bg ? g = sG(La(c.ek(a.B, e.A))) : (c = c.elements, e = new AI(c[3], c, new yI(null), e, b), e.T.B = [], b = a.A, a.A = "", QI(a, e), e = a.A, a.A = b, g = e);
        g || (g = bH(f.name(), d));
        g && iH(f, 0, d, g, !0, !1)
    }

    function jJ(a, b, c, d, e) {
        c.elements && (c = c.elements, b = new AI(c[3], c, new yI(null), d, b), b.T.B = [], b.T.A = e, fH(e, c[1]), e = a.A, a.A = "", QI(a, b), a.A = e)
    }

    function iJ(a, b, c) {
        var d = c.D,
            e = c.T,
            f = e.B || e.element.__rt,
            g = a.B.A[d];
        if (g && g.hk) null != a.A && (c = e.A.id(), a.A += pH(e.A, !1, !0) + gH(e.A), a.D[c] = e);
        else if (g && g.elements) {
            e.element && iH(e.A, 0, "jstcache", e.element.getAttribute("jstcache") || "0", !1, !0);
            null == e.element && b && b.F && b.F[2] && -1 != b.F.lc && 0 != b.F.lc && lJ(e.A, b.D, b.F.lc);
            f.push(d);
            for (var d = a.B, f = c.A, k = g.Ai, l = null == k ? 0 : k.length, m = 0; m < l; ++m)
                for (var n = k[m], p = 0; p < n.length; p += 2) {
                    var r = n[p + 1];
                    switch (n[p]) {
                        case "css":
                            var u = "string" == typeof r ? r : GF(f, r, null);
                            u &&
                                (r = d, u in r.H || (r.H[u] = !0, -1 == r.K.indexOf(u) && r.C.push(u)));
                            break;
                        case "$g":
                            (0, r[0])(f.A, f.B ? f.B.A[r[1]] : null);
                            break;
                        case "var":
                            GF(f, r, null)
                    }
                }
            null == e.element && e.A && b && mJ(e.A, b);
            "jsl" == g.elements[0] && ("jsl" != e.A.name() || b.F && b.F[2]) && mH(e.A, !0);
            c.F = g.elements;
            e = c.T;
            g = c.F;
            if (b = null == a.A) a.A = "", a.D = {}, a.F = {};
            c.B = g[3];
            fH(e.A, g[1]);
            g = a.A;
            a.A = "";
            0 != (e.A.D & 2048) ? (d = c.A.A.Ya, c.A.A.Ya = !1, QI(a, c, void 0), c.A.A.Ya = !1 !== d) : QI(a, c, void 0);
            a.A = g + a.A;
            if (b) {
                c = a.B;
                c.B && 0 != c.C.length && (b = c.C.join(""), Rc ? (c.G || (c.G =
                    tI(c)), g = c.G) : g = tI(c), g.styleSheet && !g.sheet ? g.styleSheet.cssText += b : g.textContent += b, c.C.length = 0);
                c = e.element;
                g = a.G;
                b = a.A;
                if ("" != b || "" != c.innerHTML)
                    if (d = c.nodeName.toLowerCase(), e = 0, "table" == d ? (b = "<table>" + b + "</table>", e = 1) : "tbody" == d || "thead" == d || "tfoot" == d || "caption" == d || "colgroup" == d || "col" == d ? (b = "<table><tbody>" + b + "</tbody></table>", e = 2) : "tr" == d && (b = "<table><tbody><tr>" + b + "</tr></tbody></table>", e = 3), 0 == e) c.innerHTML = b;
                    else {
                        g = g.createElement("div");
                        g.innerHTML = b;
                        for (b = 0; b < e; ++b) g = g.firstChild;
                        Sj(c);
                        for (e = g.firstChild; e; e = g.firstChild) c.appendChild(e)
                    }
                c = c.querySelectorAll ? c.querySelectorAll("[jstid]") : [];
                for (e = 0; e < c.length; ++e) {
                    g = c[e];
                    d = g.getAttribute("jstid");
                    b = a.D[d];
                    d = a.F[d];
                    g.removeAttribute("jstid");
                    for (f = b; f; f = f.F) f.element = g;
                    b.B && (g.__rt = b.B, b.B = null);
                    g.__cdn = d;
                    DI(d);
                    g.__jstcache = d.B;
                    if (b.C) {
                        for (g = 0; g < b.C.length; ++g) d = b.C[g], d.shift().apply(a, d);
                        b.C = null
                    }
                }
                a.A = null;
                a.D = null;
                a.F = null
            }
        }
    }

    function nJ(a, b, c, d) {
        var e = b.cloneNode(!1);
        if (null == b.__rt)
            for (b = b.firstChild; null != b; b = b.nextSibling) 1 == b.nodeType ? e.appendChild(nJ(a, b, c, !0)) : e.appendChild(b.cloneNode(!0));
        else e.__rt && delete e.__rt;
        e.__cdn && delete e.__cdn;
        e.__ctx && delete e.__ctx;
        e.__rjsctx && delete e.__rjsctx;
        d || ao(e, !0);
        return e
    }

    function aJ(a) {
        return null == a ? [] : ua(a) ? a : [a]
    }

    function YI(a, b, c, d) {
        var e = c[0],
            f = c[1],
            g = c[2],
            k = c[4];
        return function(c) {
            var l = b.element;
            c = aJ(c);
            var n = c.length;
            g(a.A, n);
            for (var p = d.length = 0; p < n; ++p) {
                e(a.A, c[p]);
                f(a.A, p);
                var r = GF(a, k, l);
                d.push(String(r))
            }
            return d.join(",")
        }
    }
    q.Ki = function(a, b, c, d, e) {
        var f = a.C,
            g = a.B[c + 1],
            k = g[0],
            l = g[1],
            m = g[2],
            n = a.A,
            g = a.T;
        d = aJ(d);
        var p = d.length;
        m(n.A, p);
        if (e)
            if (null != this.A) oJ(this, a, b, c, d);
            else {
                for (v = p; v < f.length; ++v) bJ(this.B, f[v], !0);
                0 < f.length && (f.length = Math.max(p, 1));
                var r = g.element;
                b = r;
                var u = !1;
                e = a.M;
                m = QG(b);
                for (v = 0; v < p || 0 == v; ++v) {
                    if (u) {
                        var t = nJ(this, r, a.D);
                        Tj(t, b);
                        b = t;
                        m.length = e + 1
                    } else 0 < v && (b = bk(b), m = QG(b)), m[e] && "*" != m[e].charAt(0) || (u = 0 < p);
                    TG(b, m, e, p, v);
                    0 == v && ao(b, 0 < p);
                    0 < p && (k(n.A, d[v]), l(n.A, v), RI(this, b, null), t = f[v], null == t ?
                        (t = f[v] = new AI(a.B, a.F, new yI(b), n, a.D), t.H = c + 2, t.I = a.I, t.M = e + 1, t.L = !0, JI(this, t)) : II(this, t), b = t.T.next || t.T.element)
                }
                if (!u)
                    for (a = bk(b); a && SG(QG(a), m, e);) c = bk(a), Yj(a), a = c;
                g.next = b
            }
        else
            for (var v = 0; v < p; ++v) k(n.A, d[v]), l(n.A, v), II(this, f[v])
    };
    q.Li = function(a, b, c, d, e) {
        var f = a.C,
            g = a.A,
            k = a.B[c + 1],
            l = k[0],
            m = k[1],
            k = a.T;
        d = aJ(d);
        if (e || !k.element || k.element.__forkey_has_unprocessed_elements) {
            e = b.A;
            var n = d.length;
            if (null != this.A) oJ(this, a, b, c, d, e);
            else {
                var p = k.element;
                b = p;
                var r = a.M,
                    u = QG(b),
                    t = [],
                    v = {},
                    x = null,
                    D;
                a: {
                    var z = this.G;
                    try {
                        D = z && z.activeElement;
                        break a
                    } catch (M) {}
                    D = null
                }
                for (var A = b, z = u; A;) {
                    RI(this, A, a.D);
                    var B = RG(A);
                    B && (v[B] = t.length);
                    t.push(A);
                    !x && D && ck(A, D) && (x = A);
                    (A = bk(A)) ? (B = QG(A), SG(B, z, r) ? z = B : A = null) : A = null
                }
                A = b.previousSibling;
                A || (A = this.G.createComment("jsfor"),
                    b.parentNode && b.parentNode.insertBefore(A, b));
                D = [];
                p.__forkey_has_unprocessed_elements = !1;
                if (0 < n)
                    for (z = 0; z < n; ++z) {
                        var Q = e[z];
                        if (Q in v) {
                            B = v[Q];
                            delete v[Q];
                            b = t[B];
                            t[B] = null;
                            if (A.nextSibling != b)
                                if (b != x) Tj(b, A);
                                else
                                    for (; A.nextSibling != b;) Tj(A.nextSibling, b);
                            D[z] = f[B]
                        } else b = nJ(this, p, a.D), Tj(b, A);
                        l(g.A, d[z]);
                        m(g.A, z);
                        TG(b, u, r, n, z, Q);
                        0 == z && ao(b, !0);
                        RI(this, b, null);
                        0 == z && p != b && (p = k.element = b);
                        A = D[z];
                        null == A ? (A = new AI(a.B, a.F, new yI(b), g, a.D), A.H = c + 2, A.I = a.I, A.M = r + 1, A.L = !0, JI(this, A) ? D[z] = A : p.__forkey_has_unprocessed_elements = !0) : II(this, A);
                        A = b = A.T.next || A.T.element
                    } else t[0] = null, f[0] && (D[0] = f[0]), ao(b, !1), TG(b, u, r, 0, 0, RG(b));
                for (Q in v) B = v[Q], (c = f[B]) && bJ(this.B, c, !0);
                a.C = D;
                for (z = 0; z < t.length; ++z) t[z] && Yj(t[z]);
                k.next = b
            }
        } else if (0 < d.length)
            for (z = 0; z < f.length; ++z) l(g.A, d[z]), m(g.A, z), II(this, f[z])
    };

    function oJ(a, b, c, d, e, f) {
        var g = b.C,
            k = b.B[d + 1],
            l = k[0],
            k = k[1],
            m = b.A;
        c = hJ(a, b, c) ? 0 : e.length;
        for (var n = 0 == c, p = b.F[2], r = 0; r < c || 0 == r && p; ++r) {
            n || (l(m.A, e[r]), k(m.A, r));
            var u = g[r] = new AI(b.B, b.F, new yI(null), m, b.D);
            u.H = d + 2;
            u.I = b.I;
            u.M = b.M + 1;
            u.L = !0;
            u.O = (b.O ? b.O + "," : "") + (r == c - 1 || n ? "*" : "") + String(r) + (f && !n ? ";" + f[r] : "");
            var t = TI(a, u);
            p && 0 < c && iH(t, 20, "jsinstance", u.O);
            0 == r && (u.T.F = b.T);
            n ? VI(a, u) : QI(a, u)
        }
    }
    q.$m = function(a, b, c) {
        b = a.A;
        c = a.B[c + 1];
        var d = a.T.element;
        this.C && a.F && a.F[2] ? $I(b, c, d, "") : GF(b, c, d)
    };
    q.an = function(a, b, c) {
        var d = a.A,
            e = a.B[c + 1];
        c = e[0];
        if (null != this.A) a = GF(d, e[1], null), c(d.A, a), b.A = iI(a);
        else {
            a = a.T.element;
            if (null == b.A) {
                e = a.__vs;
                if (!e)
                    for (var e = a.__vs = [1], f = a.getAttribute("jsvs"), f = CH(f), g = 0, k = f.length; g < k;) {
                        var l = GH(f, g),
                            m = f.slice(g, l).join(""),
                            g = l + 1;
                        e.push(HH(m))
                    }
                f = e[0]++;
                b.A = e[f]
            }
            a = GF(d, b.A, a);
            c(d.A, a)
        }
    };
    q.Ii = function(a, b, c) {
        GF(a.A, a.B[c + 1], a.T.element)
    };
    q.Wj = function(a, b, c) {
        b = a.B[c + 1];
        a = a.A;
        (0, b[0])(a.A, a.B ? a.B.A[b[1]] : null)
    };

    function lJ(a, b, c) {
        iH(a, 0, "jstcache", dI(String(c), b), !1, !0)
    }
    q.Rm = function(a, b, c) {
        b = a.A;
        var d = a.T;
        c = a.B[c + 1];
        null != this.A && a.F[2] && lJ(d.A, a.D, 0);
        d.A && c && eH(d.A, -1, null, null, null, null, c, !1);
        oI(this.B.D, c) && (d.element ? this.xg(d, c, b) : (d.C = d.C || []).push([this.xg, d, c, b]))
    };
    q.xg = function(a, b, c) {
        var d = this.B.D;
        if (!c.A.pe) {
            var e = this.B,
                e = new sH(c, e.A[b] && e.A[b].mf ? e.A[b].mf : null),
                f = new rH;
            f.D = a.element;
            b = pI(d, b, f, e);
            c.A.pe = b;
            a.element.__ctx || (a.element.__ctx = c)
        }
    };
    q.gk = function(a, b) {
        if (!b.A) {
            var c = a.T;
            a = a.A;
            c.element ? this.yg(c, a) : (c.C = c.C || []).push([this.yg, c, a]);
            b.A = !0
        }
    };
    q.yg = function(a, b) {
        a = a.element;
        a.__rjsctx || (a.__rjsctx = b)
    };

    function bJ(a, b, c) {
        if (b) {
            if (c) {
                c = b.N;
                if (null != c) {
                    for (var d in c)
                        if (0 == d.indexOf("controller:") || 0 == d.indexOf("observer:")) {
                            var e = c[d];
                            null != e && e.ra && e.ra()
                        }
                    b.N = null
                }
                if ("$t" == b.B[b.H]) {
                    d = b.A;
                    if (c = d.A.pe) {
                        c = c.nh;
                        e = a.D;
                        if (c.A) try {
                            e.B("controller_dispose", c.A.Ld), Dk(c.A)
                        } catch (f) {
                            try {
                                e.C.A(f)
                            } catch (g) {}
                        } finally {
                            e.B("controller_dispose", c.A.Ld), c.A.nh = null
                        }
                        d.A.pe = null
                    }
                    b.T.element && b.T.element.__ctx && (b.T.element.__ctx = null)
                }
            }
            null != b.G && bJ(a, b.G, !0);
            if (null != b.C)
                for (d = 0; d < b.C.length; ++d)(c = b.C[d]) && bJ(a,
                    c, !0)
        }
    }
    q.Zf = function(a, b, c, d, e) {
        var f = a.T,
            g = "$if" == a.B[c];
        if (null != this.A) d && this.C && (f.D = !0, b.C = ""), c += 2, g ? d ? QI(this, a, c) : a.F[2] && VI(this, a, c) : d ? QI(this, a, c) : VI(this, a, c), b.A = !0;
        else {
            var k = f.element;
            g && f.A && fH(f.A, 768);
            d || MI(this, f, a);
            if (e)
                if (ao(k, !!d), d) b.A || (QI(this, a, c + 2), b.A = !0);
                else if (b.A && bJ(this.B, a, "$t" != a.B[a.H]), g) {
                d = !1;
                for (g = c + 2; g < a.B.length; g += 2)
                    if (e = a.B[g], "$u" == e || "$ue" == e || "$up" == e) {
                        d = !0;
                        break
                    }
                if (d) {
                    for (; d = k.firstChild;) k.removeChild(d);
                    d = k.__cdn;
                    for (g = a.G; null != g;) {
                        if (d == g) {
                            k.__cdn = null;
                            break
                        }
                        g = g.G
                    }
                    b.A = !1;
                    a.J.length = (c - a.H) / 2 + 1;
                    a.K = 0;
                    a.G = null;
                    a.C = null;
                    b = hI(k);
                    b.length > a.I && (b.length = a.I)
                }
            }
        }
    };
    q.Lm = function(a, b, c) {
        b = a.T;
        null != b && null != b.element && GF(a.A, a.B[c + 1], b.element)
    };
    q.Pm = function(a, b, c, d, e) {
        null != this.A ? (QI(this, a, c + 2), b.A = !0) : (d && MI(this, a.T, a), !e || d || b.A || (QI(this, a, c + 2), b.A = !0))
    };
    q.bk = function(a, b, c) {
        var d = a.T.element,
            e = a.B[c + 1];
        c = e[0];
        var f = e[1],
            g = b.A,
            e = null != g;
        e || (b.A = g = new BF);
        HF(g, a.A);
        b = GF(g, f, d);
        "create" != c && "load" != c || !d ? eJ(a)["action:" + c] = b : e || (OI(d, a), b.call(d))
    };
    q.ck = function(a, b, c) {
        b = a.A;
        var d = a.B[c + 1],
            e = d[0];
        c = d[1];
        var f = d[2],
            d = d[3],
            g = a.T.element;
        a = eJ(a);
        var e = "controller:" + e,
            k = a[e];
        null == k ? a[e] = GF(b, f, g) : (c(b.A, k), d && GF(b, d, g))
    };

    function WI(a, b) {
        var c = a.element,
            d = c.__tag;
        if (null != d) a.A = d, d.reset(b || void 0);
        else if (a = d = a.A = c.__tag = new $G(c.nodeName.toLowerCase()), b = b || void 0, d = c.getAttribute("jsan")) {
            fH(a, 64);
            var d = d.split(","),
                e = d.length;
            if (0 < e) {
                a.A = [];
                for (var f = 0; f < e; f++) {
                    var g = d[f],
                        k = g.indexOf(".");
                    if (-1 == k) eH(a, -1, null, null, null, null, g, !1);
                    else {
                        var l = parseInt(g.substr(0, k), 10),
                            m = g.substr(k + 1),
                            n = null,
                            k = "_jsan_";
                        switch (l) {
                            case 7:
                                g = "class";
                                n = m;
                                k = "";
                                break;
                            case 5:
                                g = "style";
                                n = m;
                                break;
                            case 13:
                                m = m.split(".");
                                g = m[0];
                                n = m[1];
                                break;
                            case 0:
                                g = m;
                                k = c.getAttribute(m);
                                break;
                            default:
                                g = m
                        }
                        eH(a, l, g, n, null, null, k, !1)
                    }
                }
            }
            a.K = !1;
            a.reset(b)
        }
    }

    function TI(a, b) {
        var c = b.F,
            d = b.T.A = new $G(c[0]);
        fH(d, c[1]);
        !1 === b.A.A.Ya && fH(d, 1024);
        a.F && (a.F[d.id()] = b);
        b.L = !0;
        return d
    }
    q.vi = function(a, b, c) {
        var d = a.B[c + 1];
        b = a.T.A;
        var e = a.A,
            f = a.T.element;
        if (!f || "NARROW_PATH" != f.__narrow_strategy) {
            var g = d[0],
                k = d[1],
                l = d[3],
                m = d[4];
            a = d[5];
            c = !!d[7];
            if (!c || null != this.A)
                if (!d[8] || !this.C) {
                    var n = !0;
                    null != l && (n = this.C && "nonce" != a ? !0 : !!GF(e, l, f));
                    var e = n ? null == m ? void 0 : "string" == typeof m ? m : this.C ? $I(e, m, f, "") : GF(e, m, f) : null,
                        p;
                    null != l || !0 !== e && !1 !== e ? null === e ? p = null : void 0 === e ? p = a : p = String(e) : p = (n = e) ? a : null;
                    e = null !== p || null == this.A;
                    switch (g) {
                        case 6:
                            fH(b, 256);
                            e && iH(b, g, "class", p, !1, c);
                            break;
                        case 7:
                            e && jH(b, g, "class", a, n ? "" : null, c);
                            break;
                        case 4:
                            e && iH(b, g, "style", p, !1, c);
                            break;
                        case 5:
                            if (n) {
                                if (m)
                                    if (k && null !== p) {
                                        d = p;
                                        p = 5;
                                        switch (k) {
                                            case 5:
                                                k = GG(d);
                                                break;
                                            case 6:
                                                k = NG.test(d) ? d : "zjslayoutzinvalid";
                                                break;
                                            case 7:
                                                k = KG(d);
                                                break;
                                            default:
                                                p = 6, k = "sanitization_error_" + k
                                        }
                                        jH(b, p, "style", a, k, c)
                                    } else e && jH(b, g, "style", a, p, c)
                            } else e && jH(b, g, "style", a, null, c);
                            break;
                        case 8:
                            k && null !== p ? kH(b, k, a, p, c) : e && iH(b, g, a, p, !1, c);
                            break;
                        case 13:
                            k = d[6];
                            e && jH(b, g, a, k, p, c);
                            break;
                        case 14:
                        case 11:
                        case 12:
                        case 10:
                        case 9:
                            e && jH(b,
                                g, a, "", p, c);
                            break;
                        default:
                            "jsaction" == a ? (e && iH(b, g, a, p, !1, c), f && "__jsaction" in f && delete f.__jsaction) : "jsnamespace" == a ? (e && iH(b, g, a, p, !1, c), f && "__jsnamespace" in f && delete f.__jsnamespace) : a && null == d[6] && (k && null !== p ? kH(b, k, a, p, c) : e && iH(b, g, a, p, !1, c))
                    }
                }
        }
    };

    function mJ(a, b) {
        for (var c = b.B, d = 0; c && d < c.length; d += 2)
            if ("$tg" == c[d]) {
                !1 === GF(b.A, c[d + 1], null) && mH(a, !1);
                break
            }
    }

    function MI(a, b, c) {
        var d = b.A;
        if (null != d) {
            var e = b.element;
            null == e ? (mJ(d, c), -1 != c.F.lc && c.F[2] && "$t" != c.F[3][0] && lJ(d, c.D, c.F.lc), c.T.D && jH(d, 5, "style", "display", "none", !0), e = d.id(), c = 0 != (c.F[1] & 16), a.D ? (a.A += pH(d, c, !0), a.D[e] = b) : a.A += pH(d, c, !1)) : "NARROW_PATH" != e.__narrow_strategy && (c.T.D && jH(d, 5, "style", "display", "none", !0), d.apply(e))
        }
    }

    function UI(a, b, c) {
        var d = b.element;
        b = b.A;
        null != b && null != a.A && null == d && (c = c.F, 0 == (c[1] & 16) && 0 == (c[1] & 8) && (a.A += gH(b)))
    }
    q.Di = function(a, b, c) {
        if (!hJ(this, a, b)) {
            var d = a.B[c + 1];
            b = a.A;
            c = a.T.A;
            var e = d[3],
                f = !!b.A.Fa,
                d = GF(b, d[2], a.T.element);
            a = WF(d, e, f);
            e = YF(d, e, f);
            if (f != a || f != e) c.G = !0, iH(c, 0, "dir", a ? "rtl" : "ltr");
            b.A.Fa = a
        }
    };
    q.Ei = function(a, b, c) {
        if (!hJ(this, a, b)) {
            var d = a.B[c + 1];
            b = a.A;
            c = a.T.element;
            if (!c || "NARROW_PATH" != c.__narrow_strategy) {
                a = a.T.A;
                var e = d[2],
                    f = d[3],
                    g = d[4],
                    d = !!b.A.Fa,
                    f = f ? GF(b, f, c) : null;
                c = "rtl" == GF(b, e, c);
                e = null != f ? YF(f, g, d) : d;
                if (d != c || d != e) a.G = !0, iH(a, 0, "dir", c ? "rtl" : "ltr");
                b.A.Fa = c
            }
        }
    };
    q.Ci = function(a, b) {
        hJ(this, a, b) || (b = a.A, a = a.T.element, a && "NARROW_PATH" == a.__narrow_strategy || (b.A.Fa = !!b.A.Fa))
    };
    q.yi = function(a, b, c, d, e) {
        var f = a.B[c + 1],
            g = f[0],
            k = a.A;
        d = String(d);
        c = a.T;
        var l = !1,
            m = !1;
        3 < f.length && null != c.A && !hJ(this, a, b) && (m = f[3], f = !!GF(k, f[4], null), l = 7 == g || 2 == g || 1 == g, m = null != m ? GF(k, m, null) : WF(d, l, f), l = m != f || f != YF(d, l, f)) && (null == c.element && mJ(c.A, a), null == this.A || !1 !== c.A.G) && (iH(c.A, 0, "dir", m ? "rtl" : "ltr"), l = !1);
        MI(this, c, a);
        if (e) {
            if (null != this.A) {
                if (!hJ(this, a, b)) {
                    b = null;
                    l && (!1 !== k.A.Ya ? (this.A += '<span dir="' + (m ? "rtl" : "ltr") + '">', b = "</span>") : (this.A += m ? "\u202b" : "\u202a", b = "\u202c" + (m ? "\u200e" :
                        "\u200f")));
                    switch (g) {
                        case 7:
                        case 2:
                            this.A += d;
                            break;
                        case 1:
                            this.A += AG(d);
                            break;
                        default:
                            this.A += sG(d)
                    }
                    null != b && (this.A += b)
                }
            } else {
                b = c.element;
                switch (g) {
                    case 7:
                    case 2:
                        OG(b, d);
                        break;
                    case 1:
                        g = AG(d);
                        OG(b, g);
                        break;
                    default:
                        g = !1;
                        e = "";
                        for (k = b.firstChild; k; k = k.nextSibling) {
                            if (3 != k.nodeType) {
                                g = !0;
                                break
                            }
                            e += k.nodeValue
                        }
                        if (k = b.firstChild) {
                            if (g || e != d)
                                for (; k.nextSibling;) Yj(k.nextSibling);
                            3 != k.nodeType && Yj(k)
                        }
                        b.firstChild ? e != d && (b.firstChild.nodeValue = d) : b.appendChild(b.ownerDocument.createTextNode(d))
                }
                "TEXTAREA" !=
                b.nodeName && "textarea" != b.nodeName || b.value === d || (b.value = d)
            }
            UI(this, c, a)
        }
    };

    function RI(a, b, c) {
        aI(a.G, b, c);
        return b.__jstcache
    }

    function pJ(a) {
        this.method = a;
        this.A = this.la = 0
    }
    var LI = {},
        qJ = !1;

    function rJ() {
        if (!qJ) {
            qJ = !0;
            var a = EI.prototype,
                b = function(a) {
                    return new pJ(a)
                };
            LI.$a = b(a.vi);
            LI.$c = b(a.yi);
            LI.$dh = b(a.Ci);
            LI.$dc = b(a.Di);
            LI.$dd = b(a.Ei);
            LI.display = b(a.Zf);
            LI.$e = b(a.Ii);
            LI["for"] = b(a.Ki);
            LI.$fk = b(a.Li);
            LI.$g = b(a.Wj);
            LI.$ia = b(a.bk);
            LI.$ic = b(a.ck);
            LI.$if = b(a.Zf);
            LI.$o = b(a.Mk);
            LI.$rj = b(a.gk);
            LI.$r = b(a.Lm);
            LI.$sk = b(a.Pm);
            LI.$s = b(a.K);
            LI.$t = b(a.Rm);
            LI.$u = b(a.Vm);
            LI.$ua = b(a.Wm);
            LI.$uae = b(a.Xm);
            LI.$ue = b(a.Ym);
            LI.$up = b(a.Zm);
            LI["var"] = b(a.$m);
            LI.$vs = b(a.an);
            LI.$c.la = 1;
            LI.display.la = 1;
            LI.$if.la =
                1;
            LI.$sk.la = 1;
            LI["for"].la = 4;
            LI["for"].A = 2;
            LI.$fk.la = 4;
            LI.$fk.A = 2;
            LI.$s.la = 4;
            LI.$s.A = 3;
            LI.$u.la = 3;
            LI.$ue.la = 3;
            LI.$up.la = 3;
            AF.runtime = FF;
            AF.and = MF;
            AF.bidiCssFlip = dG;
            AF.bidiDir = VF;
            AF.bidiExitDir = XF;
            AF.bidiLocaleDir = LF;
            AF.url = kG;
            AF.urlToString = lG;
            AF.urlParam = mG;
            AF.hasUrlParam = nG;
            AF.bind = iG;
            AF.debug = RF;
            AF.ge = PF;
            AF.gt = NF;
            AF.le = QF;
            AF.lt = OF;
            AF.has = gG;
            AF.size = hG;
            AF.range = UF;
            AF.string = eG;
            AF["int"] = fG
        }
    }

    function KI(a) {
        var b = a.T.element;
        if (!b || !b.parentNode || "NARROW_PATH" != b.parentNode.__narrow_strategy || b.__narrow_strategy) return !0;
        for (b = 0; b < a.B.length; b += 2) {
            var c = a.B[b];
            if ("for" == c || "$fk" == c && b >= a.H) return !0
        }
        return !1
    };

    function sJ(a, b) {
        this.C = a;
        this.B = new BF;
        this.B.B = this.C.F;
        this.A = null;
        this.D = b
    }

    function dE(a, b) {
        if (a.A) {
            var c = a.B,
                d = a.A,
                e = a.C;
            a = a.D;
            rJ();
            for (var f = e.I, g = f.length - 1; 0 <= g; --g) {
                var k = f[g];
                HI(d, a, k.A.T.element, k.A.D) && f.splice(g, 1)
            }
            f = "rtl" == KF(d);
            c.A.Fa = f;
            c.A.Ya = !0;
            k = null;
            (g = d.__cdn) && g.B != xI && "no_key" != a && (f = CI(g, a, null)) && (g = f, k = "rebind", f = new EI(e, void 0, void 0), HF(g.A, c), g.T.A && !g.L && d == g.T.element && g.T.A.reset(a), II(f, g));
            if (null == k) {
                e.document();
                Jj(d);
                var f = new EI(e, void 0, void 0),
                    e = RI(f, d, null),
                    l = "$t" == e[0] ? 1 : 0,
                    k = 0;
                if ("no_key" != a && a != d.getAttribute("id")) {
                    var m = !1,
                        g = e.length -
                        2;
                    if ("$t" == e[0] && e[1] == a) k = 0, m = !0;
                    else if ("$u" == e[g] && e[g + 1] == a) k = g, m = !0;
                    else
                        for (var n = hI(d), g = 0; g < n.length; ++g)
                            if (n[g] == a) {
                                e = bI(a);
                                l = g + 1;
                                k = 0;
                                m = !0;
                                break
                            }
                }
                g = new BF;
                HF(g, c);
                g = new AI(e, null, new yI(d), g, a);
                g.H = k;
                g.I = l;
                g.T.B = hI(d);
                c = !1;
                m && "$t" == e[k] && (WI(g.T, a), m = f.B.A[a], c = null == m || null == m.Od ? !1 : m.Od != d.getAttribute("jssc"));
                c ? iJ(f, null, g) : JI(f, g)
            }
        }
        b && b()
    }
    sJ.prototype.remove = function() {
        var a = this.A;
        if (null != a) {
            var b = a.parentElement;
            if (null == b || !b.__cdn) {
                b = this.C;
                if (a) {
                    var c = a.__cdn;
                    c && (c = CI(c, this.D)) && bJ(b, c, !0)
                }
                null != a.parentNode && a.parentNode.removeChild(a);
                this.A = null;
                this.B = new BF;
                this.B.B = this.C.F
            }
        }
    };

    function tJ(a, b) {
        sJ.call(this, a, b)
    }
    H(tJ, sJ);

    function uJ(a, b) {
        sJ.call(this, a, b)
    }
    H(uJ, tJ);

    function vJ(a) {
        sJ.call(this, a, wJ);
        var b = wJ;
        if (!(b in a.A) || a.A[b].hk) {
            var b = wJ,
                c = ["div", , 1, 0, [" ", ["canvas", , , 1], " ", ["div", , , 2], " ", ["div", , , 3], " "]],
                d = xJ();
            if (d)
                for (var e = 0; e < d.length; ++e) d[e] && SH(d[e], b + " " + String(e));
            wI(c, d);
            a = a.A;
            var f;
            d = {
                fc: 0
            };
            if ("array" == ta(d)) f = d;
            else {
                e = [];
                for (f in d) e[d[f]] = f;
                f = e
            }
            a[b] = {
                Mm: 0,
                elements: c,
                Ai: [
                    ["css", ".widget-scene{width:100%;height:100%;overflow:hidden;position:absolute;z-index:0;background-color:black}", "css", ".keynav-mode .widget-scene:focus::after{content:'';border:2px solid #4d90fe;box-sizing:border-box;height:100%;pointer-events:none;position:absolute;width:100%;z-index:1}",
                        "css", ".print-mode .widget-scene:focus::after{display:none}", "css", ".widget-scene-effects{position:absolute;left:0;top:0;z-index:2}", "css", ".widget-scene-imagery-render{position:absolute;left:0;top:0;z-index:1;background-color:black}", "css", ".widget-scene-imagery-iframe{position:absolute}", "css", ".widget-scene .canvas-renderer{position:absolute;left:0;top:0}", "css", ".widget-scene-canvas{position:absolute;left:0;top:0;background-color:black}", "css", ".widget-scene-capture-canvas{position:relative;z-index:3}",
                        "css", ".tile-image-3d{-webkit-perspective:1000;-webkit-backface-visibility:hidden;perspective:1000;backface-visibility:hidden;-moz-perspective:1000;-moz-backface-visibility:hidden;-o-perspective:1000;-o-backface-visibility:hidden}", "css", ".accelerated{-webkit-transform:translateZ(0);transform:translateZ(0)}", "css", "@media print{.widget-scene{background:white;position:static;overflow:visible}.widget-scene-canvas{display:block;background:white}.app-globe-mode .widget-scene-canvas{background:black}.widget-scene-imagery-render{position:relative;background:white;z-index:4}.widget-scene-imagery-iframe{position:relative;left:50% !important;transform:translateX(-50%);-webkit-transform:translateX(-50%)}.widget-scene .canvas-renderer,.widget-scene .canvas-container,.widget-scene canvas{position:static !important}.canvas-renderer+.widget-scene-canvas{display:none !important}.widget-scene-capture-canvas+.widget-scene-canvas,.widget-scene-capture-canvas+.canvas-renderer{display:none !important}.widget-scene-canvas{width:100% !important;height:auto !important;-webkit-transform:none !important;transform:none !important}}",
                        "css", ".print-mode .widget-scene{background:white;position:static;overflow:visible}", "css", ".print-mode .widget-scene-canvas{display:block;background:white}", "css", ".print-mode .app-globe-mode .widget-scene-canvas{background:black}", "css", ".print-mode .widget-scene-imagery-render{position:relative;background:white;z-index:4}", "css", ".print-mode .widget-scene-imagery-iframe{position:relative;left:50% !important;transform:translateX(-50%);-webkit-transform:translateX(-50%)}", "css", ".print-mode .widget-scene .canvas-renderer,.print-mode .widget-scene .canvas-container,.print-mode .widget-scene canvas{position:static !important}",
                        "css", ".print-mode .canvas-renderer+.widget-scene-canvas{display:none !important}", "css", ".print-mode .widget-scene-capture-canvas+.widget-scene-canvas,.print-mode .widget-scene-capture-canvas+.canvas-renderer{display:none !important}"
                    ]
                ],
                oc: f,
                mf: null,
                async: !1,
                Od: null
            }
        }
    }
    H(vJ, uJ);
    vJ.prototype.fill = function(a) {
        a = oG(a);
        this.B.A[this.C.A[this.D].oc[0]] = a
    };
    var wJ = "t-nrD2PAT7leI";

    function xJ() {
        return [
            ["$t", "t-nrD2PAT7leI", "var", function(a) {
                return a.va = TF(a.fc, "", -4) + "." + TF(a.fc, "", -5)
            }, "var", function(a) {
                return a.action = TF(a.fc, !1, -2) ? "click: " + a.va + ";dblclick: " + a.va + ";mousedown: " + a.va + ";mousemove: " + a.va + ";mouseup: " + a.va + ";mouseover: " + a.va + ";mouseout: " + a.va + ";touchstart: " + a.va + ";touchmove: " + a.va + ";touchend: " + a.va + ";pointerdown: " + a.va + ";pointermove: " + a.va + ";pointerup: " + a.va + ";pointercancel: " + a.va + ";MSPointerDown: " + a.va + ";MSPointerMove: " + a.va + ";MSPointerUp: " +
                    a.va + ";MSPointerCancel: " + a.va + ";contextmenu: " + a.va + ";keydown: " + a.va + ";keyup: " + a.va + ";" + (TF(a.fc, !1, -3) ? "wheel: " + a.va + ";mousewheel: " + a.va + ";DOMMouseScroll: " + a.va + ";" : "") : ""
            }, "$a", [7, , , , , "widget-scene"], "$a", [0, , , , function(a) {
                return TF(a.fc, "", -6)
            }, "aria-label", , , 1], "$a", [5, 5, , , function(a) {
                return a.Fa ? dG("cursor", TF(a.fc, "", -1)) : TF(a.fc, "", -1)
            }, "cursor", , , 1], "$a", [0, , , , function(a) {
                return a.action
            }, "jsaction", , , 1], "$a", [0, , , , "main", "role"], "$a", [0, , , , "0", "tabindex"]],
            ["$a", [7, , , , , "widget-scene-canvas", , 1]],
            ["$a", [7, , , , , "widget-scene-imagery-render", , 1]],
            ["$a", [7, , , , , "widget-scene-effects", , 1], "$a", [7, , , , , "noprint", , 1]]
        ]
    };

    function yJ(a, b) {
        co(b)
    };
    var zJ = Fy();
    var AJ = Fy();

    function BJ(a, b, c, d, e, f, g, k, l, m, n) {
        this.F = a;
        this.D = b;
        this.A = c;
        this.C = d;
        this.B = e;
        this.G = f;
        this.H = g;
        this.I = k;
        this.M = l;
        this.N = m;
        this.K = n
    }
    BJ.prototype.L = function(a) {
        return new BJ(this.F - a.F, this.D - a.D, this.A - a.A, this.C - a.C, this.B - a.B, this.G - a.G, this.H - a.H, this.I - a.I, a.M, this.N, this.K)
    };

    function CJ(a) {
        return Infinity == a ? 0 : a
    }
    BJ.prototype.J = function() {
        var a = .001 * this.B,
            b = CJ(this.A / a),
            c = Math.max(this.C - this.A, 0),
            a = CJ(c / a),
            d = this.D / this.A,
            e = {};
        e.f = this.F.toString();
        e.cf = this.A.toString();
        e.tf = this.C.toString();
        isNaN(b) || (e.fps = b.toFixed(1), b = this.G / this.B, isNaN(b) && (b = 0), e.stp = b.toFixed(3));
        e.df = c.toFixed(0);
        isNaN(a) || (e.dfps = a.toFixed(1));
        !isNaN(d) && isFinite(d) && (e.ms = .05 > d ? "1" : "0");
        e.pr = this.N.toFixed(2).toString();
        null != this.K && (e.wr = this.K);
        e.crt = this.B.toString();
        e.tr = this.H.toString();
        e.tp = this.I.toString();
        e.fsd =
            this.M.toFixed(2).toString();
        return e
    };

    function DJ(a) {
        this.A = a
    }
    DJ.prototype.B = function() {
        var a = this.A;
        return new BJ(this.A.aa, this.A.ha, this.A.ga, Math.floor(this.A.L / tj), this.A.L, this.A.Y, this.A.C, this.A.$, Math.sqrt(a.K - a.D * a.D), w.devicePixelRatio || 1, mE || void 0)
    };

    function EJ(a, b, c, d) {
        this.A = a;
        this.B = b;
        this.D = c || 0;
        this.C = d || 0
    }
    EJ.prototype.L = function(a) {
        if (!this.A || !a.A) return new EJ(null, this.B);
        var b = a.A,
            c = this.A,
            d = rl(),
            e = rl(),
            f = xe(c),
            c = we(c);
        dm(we(b), xe(b), 0, d, 1);
        dm(c, f, 0, e, 1);
        a: {
            switch (this.B) {
                case 3:
                    b = 3396E3;
                    break a;
                case 2:
                    b = 1737100;
                    break a
            }
            b = 6371010
        }
        return new EJ(this.A, this.B, b * Math.acos(zl(e, d)), ye(this.A) - ye(a.A))
    };
    EJ.prototype.J = function() {
        var a = {};
        if (null == this.A) return a;
        a.sca = Math.round(this.C) + "";
        a.scm = Math.round(this.D) + "";
        return a
    };

    function FJ(a) {
        this.C = a;
        this.A = Gy(Ky)
    }
    FJ.prototype.B = function() {
        var a = this.A.get() ? L(this.A.get(), 7) : 1;
        return this.C.get() ? new EJ(qe(this.C.get()), a) : new EJ(null, a)
    };

    function GJ(a, b, c, d) {
        this.A = a;
        this.F = b;
        this.D = c;
        this.B = d;
        this.C = !1
    };
    var HJ = "ptrdown ptrhover ptrout ptrup dragstart drag dragend".split(" ");

    function IJ(a) {
        this.H = !0;
        this.I = a;
        this.F = null;
        this.B = {};
        this.G = [];
        this.K = this.M = 0;
        this.J = -1;
        this.A = 0;
        this.C = !1;
        this.D = new GE(-1, -1);
        for (a = 0; a < HJ.length; ++a) {
            var b = HJ[a],
                c = E(this.L, this, b);
            sF(this.I, b, !0, c, !1, void 0)
        }
    }
    var JJ = ["ptrdown", "ptrhover", "ptrup"],
        KJ = [1, 0];

    function LJ(a, b, c, d, e) {
        var f = a.M++;
        e = e ? a.J-- : a.K++;
        d = new MJ(b, c, d, e, f);
        a.G[f] = d;
        b = "" + b + ":" + c;
        c = a.B[b];
        c ? a.C && (c = wb(c), a.B[b] = c) : (c = [], a.B[b] = c);
        c.push(d);
        return f
    }

    function NJ(a, b) {
        var c = a.G[b];
        if (c) {
            for (var d = "" + c.rb + ":" + c.A, e = a.B[d], f = 0; f < e.length; ++f)
                if (c == e[f]) {
                    a.C && (e = wb(e), a.B[d] = e);
                    e.splice(f, 1);
                    break
                }
            delete a.G[b]
        }
    }

    function EE(a, b) {
        0 != a.A && (0 < a.A && a.A--, b && (b.x = a.D.x, b.y = a.D.y))
    }
    IJ.prototype.L = function(a, b, c) {
        if (this.H) {
            if ("ptrhover" == a && (this.D.x = c.x, this.D.y = c.y, 0 != this.A)) return;
            var d = this.F && this.F.B.na,
                d = d && sb(JJ, a) ? JD(d, c.x, c.y) : null;
            OJ(this, a, c, d, b)
        }
    };

    function OJ(a, b, c, d, e) {
        a.C = !0;
        c = new GJ(c, b, d, e);
        d = d ? d.A() : KJ;
        e = [];
        for (var f = 0; f < d.length; ++f) {
            var g = a.B["" + b + ":" + d[f]];
            g && e.push.apply(e, g)
        }
        Ab(e, function(a, b) {
            return a.zg - b.zg
        });
        for (f = 0; f < e.length; ++f) e[f].gb(c);
        a.C = !1
    }

    function MJ(a, b, c, d, e) {
        this.rb = a;
        this.A = b;
        this.gb = c;
        this.zg = d;
        this.id = e
    };

    function PJ(a, b, c) {
        this.M = !1;
        this.D = a;
        this.K = b;
        this.L = !!c;
        this.J = [];
        this.B = this.C = !1;
        ++QJ;
        this.fe();
        a = this.K;
        sb(a.B, this) || a.B.push(this)
    }
    H(PJ, GC);
    var QJ = 0;

    function RJ(a) {
        a.B || (a.reset(), a.B = !0)
    }
    PJ.prototype.ea = function() {
        for (var a = 0; a < this.J.length; ++a) NJ(this.D, this.J[a]);
        this.J.length = 0;
        this.reset();
        tb(this.K.B, this)
    };

    function SJ(a, b, c, d) {
        b = LJ(a.D, b, c, d, a.L);
        a.J.push(b)
    }

    function TJ(a, b) {
        if (!a.C) {
            var c;
            if (c = !b.C)
                if (c = a.K, c.A || !sb(c.B, a)) c = !1;
                else {
                    for (var d = 0; d < c.B.length; ++d) {
                        var e = c.B[d];
                        e !== a && RJ(e)
                    }
                    c.A = a;
                    c = !0
                }
            c && (b.C || (b.C = !0), a.C = !0)
        }
    }
    PJ.prototype.reset = function() {
        if (this.C && this.C) {
            this.C = !1;
            var a = this.K;
            if (a.A && a.A === this) {
                for (var b = 0; b < a.B.length; ++b) {
                    var c = a.B[b];
                    c !== this && c.B && (c.B = !1)
                }
                a.A = null
            }
        }
    };

    function UJ(a, b, c, d, e, f, g) {
        this.O = d;
        this.N = e;
        this.P = f;
        this.I = c;
        this.H = null;
        this.A = !1;
        this.F = this.G = null;
        PJ.call(this, a, b, g)
    }
    H(UJ, PJ);
    UJ.prototype.fe = function() {
        SJ(this, "ptrdown", this.O, E(this.X, this));
        SJ(this, "dragstart", 0, E(this.U, this));
        SJ(this, "ptrup", 0, E(this.Y, this))
    };

    function VJ(a) {
        a.H = null;
        a.G = null;
        null != a.F && (Oy(a.F), a.F = null);
        a.A = !1
    }
    UJ.prototype.U = function() {
        VJ(this)
    };
    UJ.prototype.X = function(a) {
        var b = "touchstart" === a.A.type;
        if (WJ(this, a) || b) this.G && VJ(this), this.A ? (this.A = !1, this.G = a.A) : (this.A = !0, XJ(this, a.B), this.H = a.D)
    };
    UJ.prototype.Y = function(a) {
        var b = "touchend" === a.A.type;
        if ((WJ(this, a) || b) && this.G)
            if (a.C || this.B) VJ(this);
            else {
                var b = new wo(this.I, "click_2"),
                    c = this.P.A,
                    d = new GJ(a.A, a.F, this.H, b);
                ZE(c, d.A);
                var e = d.D,
                    f = d.A,
                    d = d.B;
                d.ob("scene", "click_scene");
                aF(c, f.x, f.y, c.J, d, e);
                c.B.Qg(eE(f), d);
                b.done("main-actionflow-branch");
                TJ(this, a);
                VJ(this);
                this.reset()
            }
    };

    function XJ(a, b) {
        a.F = Ny(E(function() {
            this.A = !1
        }, a), 250, b, "sceneDblClick")
    }

    function WJ(a, b) {
        b = b.A;
        switch (a.N) {
            case 0:
                return Lk(b);
            case 1:
                return Kk(b, 2) || Kk(b, 0) && !Lk(b);
            default:
                return !1
        }
    };

    function YJ(a, b, c, d, e) {
        this.N = c;
        this.H = d;
        this.I = this.G = !1;
        this.A = this.F = null;
        PJ.call(this, a, b, e)
    }
    H(YJ, PJ);
    q = YJ.prototype;
    q.fe = function() {
        SJ(this, "ptrdown", this.N, E(this.tk, this));
        SJ(this, "ptrup", 0, E(this.uk, this));
        SJ(this, "dragstart", 0, E(this.sk, this))
    };
    q.reset = function() {
        YJ.V.reset.call(this);
        this.I = this.G = !1;
        this.F && (NJ(this.D, this.F), this.F = null);
        this.A && (NJ(this.D, this.A), this.A = null)
    };
    q.tk = function(a) {
        a = a.A;
        this.B || "mousedown" === a.type && !Kk(a, 0) || (this.I = this.G = !0)
    };
    q.uk = function(a) {
        var b = a.A;
        if ("mouseup" !== b.type || Kk(b, 0)) this.G && this.H.B(a), this.reset()
    };
    q.sk = function(a) {
        if (!this.B && this.G && (this.G = !1, this.I && !a.C)) {
            var b = E(this.Zj, this);
            this.F = LJ(this.D, "drag", 0, b, this.L);
            b = E(this.Yj, this);
            this.A = LJ(this.D, "dragend", 0, b, this.L);
            if (null === this.F || null === this.A) this.reset();
            else {
                this.I = !1;
                TJ(this, a);
                b = this.H.A;
                ZE(b, a.A);
                var c = a.A,
                    d = a.B;
                b.da = !0;
                b.D.A();
                d.ob("scene", "move_camera");
                d.ua("dr0");
                d.la("dragging-branch");
                aF(b, c.x, c.y, !0, d, a.D);
                b.F.A++;
                a = eE(c);
                b.B.Ue(a, d);
                b.na = a;
                b.$ = a
            }
        }
    };
    q.Zj = function(a) {
        if (!this.B && this.C) {
            var b = this.H.A,
                c = a.A,
                d = eE(c);
            if (c.touches) {
                if (c = a.B, b.B.G)
                    if ("touchstart" == d.type || "touchend" == d.type) b.B.Ue(d, c);
                    else {
                        var e = d.me;
                        if (e)
                            if (b.Ec()) {
                                var f = b.$.me;
                                1 < Math.abs(e - f) && (e = Math.log(e / f) / Math.log(2), b.B.G.Ed(e, c, d.x, d.y))
                            } else if (f = b.na.me, e = Math.round(Math.log(e / f) / Math.log(2))) b.B.G.Ed(e, c, d.x, d.y), b.na = d;
                        b.B.Ve(d, c)
                    }
            } else b.B.Ve(d, a.B);
            b.$ = d;
            a.C || (a.C = !0)
        }
    };
    q.Yj = function(a) {
        if (!this.B) {
            if (this.C) {
                a.C || (a.C = !0);
                var b = this.H.A,
                    c = a.A,
                    d = a.B;
                b.da = !1;
                b.D.start(d);
                EE(b.F);
                b.B.Sg(eE(c), d);
                aF(b, c.x, c.y, !1, d, a.D);
                d.ua("dr1");
                d.done("dragging-branch")
            }
            this.reset()
        }
    };

    function ZJ(a, b, c, d, e, f, g) {
        this.P = d;
        this.I = f;
        this.O = e;
        this.N = c;
        this.G = this.A = null;
        this.F = !1;
        this.H = null;
        PJ.call(this, a, b, g)
    }
    H(ZJ, PJ);
    q = ZJ.prototype;
    q.fe = function() {
        SJ(this, "ptrdown", this.P, E(this.wk, this));
        SJ(this, "dragstart", 0, E(this.vk, this));
        SJ(this, "ptrup", 0, E(this.xk, this))
    };
    q.vk = function() {
        this.A && $J(this)
    };
    q.wk = function(a) {
        if (aK(this, a))
            if (this.A && !this.F && $J(this), this.F) $J(this);
            else {
                this.F = !0;
                var b = a.B;
                this.H = Ny(E(this.Xj, this), 250, b, "sceneExclusiveClick");
                this.A = a
            }
    };
    q.xk = function(a) {
        if (aK(this, a))
            if (this.A)
                if (this.F) this.G = a;
                else {
                    if (!a.C && !this.B) {
                        var b = bK(this, this.A, a);
                        $E(this.I.A, b) && TJ(this, a);
                        b.B.done("main-actionflow-branch")
                    }
                    $J(this)
                }
        else $J(this)
    };
    q.Xj = function() {
        this.F = !1;
        if (this.G && this.A) {
            if (!this.G.C && !this.B) {
                var a = bK(this, this.A, this.G);
                $E(this.I.A, a) && TJ(this, this.G);
                a.B.done("main-actionflow-branch")
            }
            $J(this)
        }
    };

    function bK(a, b, c) {
        a = new wo(a.N, "click_1");
        return new GJ(b.A, c.F, b.D, a)
    }

    function $J(a) {
        a.reset();
        null != a.H && (Oy(a.H), a.H = null);
        a.F = !1;
        a.A = null;
        a.G = null
    }

    function aK(a, b) {
        b = b.A;
        switch (a.O) {
            case 0:
                return Lk(b);
            case 1:
                return Kk(b, 2) || Kk(b, 0) && !Lk(b);
            default:
                return !1
        }
    };

    function cK(a, b, c, d, e) {
        this.G = c;
        this.F = d;
        this.A = null;
        PJ.call(this, a, b, e)
    }
    H(cK, PJ);
    q = cK.prototype;
    q.fe = function() {
        SJ(this, "ptrhover", 0, E(this.$j, this));
        SJ(this, "ptrdown", 0, E(this.yk, this));
        SJ(this, "ptrout", 0, E(this.zk, this));
        SJ(this, "ptrup", this.G, E(this.Ak, this))
    };
    q.$j = function(a) {
        var b = a.D,
            c, d = this.G,
            d = !b && (0 == d || 1 == d),
            e = !(!b || !b.D(this.G));
        c = d || e;
        d = b && this.A && this.A.C(b);
        e = (e = !this.A && c) || this.A && c && !d;
        c = this.A && b && d;
        !this.A || b && d || (this.F.A(a), this.A = null);
        e && (a.B.Wc() || a.B.ob("scene_hover", "hover_on_map"), this.A = b, bF(this.F.B, a));
        c && this.F.C(a)
    };
    q.yk = function(a) {
        this.A && (this.F.A(a), this.A = null)
    };
    q.Ak = function(a) {
        this.A = a.D;
        bF(this.F.B, a)
    };
    q.zk = function(a) {
        !this.B && this.A && (this.F.A(a), this.A = null)
    };

    function dK(a, b) {
        this.C = a;
        this.D = b;
        this.B = [];
        this.A = null
    }

    function eK(a, b, c) {
        b = new UJ(a.C, a, a.D, 0, y(c) ? c : 0, b, void 0);
        a.A && RJ(b);
        return b
    };

    function fK(a) {
        Ek.call(this, "RenderStart", a)
    }
    H(fK, Ek);

    function gK(a, b, c) {
        var d = c || w.document;
        if (d) {
            var e = null;
            c = null;
            for (var f = 0; f < hK.length; f += 2)
                if (y(d[hK[f]])) {
                    e = hK[f];
                    c = hK[f + 1];
                    break
                }
            e && c && (f = function() {
                a(!d[e])
            }, b ? b.listen(d, c, f) : Vk(d, c, f))
        }
    }
    var hK = "hidden visibilitychange webkitHidden webkitvisibilitychange mozHidden mozvisibilitychange msHidden msvisibilitychange".split(" ");

    function iK(a, b, c) {
        il.call(this);
        this.D = new wq(this);
        Ck(this, Fa(Dk, this.D));
        this.A = a;
        this.F = !!c;
        this.B = null;
        this.C = !1;
        jK(this, b)
    }
    H(iK, il);

    function jK(a, b) {
        gK(function(b) {
            b && fp(a)
        }, a.D, b)
    }

    function fp(a) {
        if (a.B && !a.C) {
            var b = a.A;
            b.G.push(a);
            ZD(b.B);
            a.C = !0
        }
    }
    iK.prototype.H = function() {
        this.C = !1;
        !this.wa() && this.B && (F(), this.dispatchEvent(new fK(this)), this.B && this.B.Pa(), this.dispatchEvent(new fE(this, F())), this.F && fp(this))
    };

    function kK(a, b, c, d, e, f, g, k, l, m, n, p, r, u, t, v) {
        this.M = !1;
        this.width = Hy(WD);
        this.width.listen(this.ga, this);
        this.height = Hy(WD);
        this.height.listen(this.ga, this);
        this.S = Gy(Iy);
        this.H = Gy(Iy);
        this.B = Gy(Ky);
        this.B.listen(this.Ua, this);
        this.J = Gy(zJ, YB(d));
        this.O = Gy(VD);
        this.ja = Gy(VD);
        this.P = Gy(HE);
        this.F = Gy(JE);
        this.X = Gy(AJ);
        this.da = Gy(LE);
        this.Ta = Gy(KE);
        this.aa = Gy(IE);
        this.Y = new HC(f);
        this.$ = g;
        Lo(l, "render", new DJ(g));
        m = new FJ(this.H);
        By(m.A, this.B, n);
        Lo(l, "camera_change", m);
        this.L = Ic(b, 0);
        this.U = O(b, 81) ||
            "scene";
        this.N = O(b, 82) || "viewport";
        this.C = f = new pF(f, this.U, this.N, g);
        this.D = new uF;
        this.canvas = null;
        this.G = new vJ(e);
        this.D.data[1] = this.L;
        this.D.data[2] = this.L;
        this.D.data[3] = this.U;
        this.D.data[4] = this.N;
        e = lK(this);
        this.D.data[5] = e;
        e = this.G;
        var x = e.C;
        m = e.D;
        if (x.document()) {
            var D = x.A[m];
            if (D && D.elements) {
                var z = D.elements[0],
                    x = x.document().createElement(z);
                1 != D.Mm && x.setAttribute("jsl", "$u " + m + ";");
                m = x
            } else m = null
        } else m = null;
        e.A = m;
        e.A && (e.A.__attached_template = e);
        a && a.appendChild(e.A);
        m = "rtl" == KF(e.A);
        e.B.A.Fa = m;
        this.G.fill(this.D);
        dE(this.G);
        e = Kj("canvas", void 0, a)[0];
        if (this.canvas = c.canvas) m = this.canvas.A, m.id = e.id, m.className = e.className, (D = e.parentNode) && D.replaceChild(m, e);
        e = Kj("div", "widget-scene-imagery-render", a);
        (c = c.B) && 1 == e.length && (m = e[0], c.id = m.id, c.className = m.className, e = e[0], (m = e.parentNode) && m.replaceChild(c, e));
        c = null;
        a = Kj("div", "widget-scene-effects", a);
        1 == a.length && (c = a[0]);
        this.xa = new nF(c || document.createElement("div"));
        this.xa.bind(this.S, this.P, n);
        b = Ic(b, 2) && !Ic(b, 7) &&
            1 === this.J.get();
        b = Hy(VD, !!b);
        By(this.O, b, n);
        this.K = new IJ(f);
        this.K.H = !1;
        this.I = new dK(this.K, l);
        this.Wa = t.B || new iK(g);
        var A;
        this.canvas ? A = new cE(this.G, this.D, this.canvas, this.$, YB(d), p) : A = new oF(this.G, this.D, this.$);
        this.view = A;
        d = this.A = new PE(0, u, 0, v, this.Wa, d, this.K, k, 0, this, n, 0, this.view, r);
        g = this.aa;
        By(d.Ga, this.ja, n);
        By(d.ha, g, n);
        By(this.view.B, this.A.I, n);
        By(this.view.C, this.A.Xe, n);
        By(this.A.width, this.width, n);
        By(this.A.height, this.height, n);
        By(this.A.Db, this.O, n);
        By(this.S, this.A.S,
            n);
        By(this.H, this.A.G, n);
        By(this.B, this.A.C, n);
        By(this.P, this.A.Fb, n);
        By(this.F, this.A.Pe, n);
        By(this.X, Gy(AJ, this.A), n);
        By(this.da, this.A.Zb, n);
        By(this.Ta, this.A.Pb, n);
        this.L && (n = E(this.A.Sk, this.A), sF(this.C, "ptrdown", !1, n, !1, void 0), n = E(this.A.Vk, this.A), sF(this.C, "ptrup", !1, n, !1, void 0), n = E(this.A.Tk, this.A), sF(this.C, "ptrin", !0, n, !1, void 0), n = E(this.A.Uk, this.A), sF(this.C, "ptrout", !0, n, !1, void 0), n = E(this.A.jl, this.A), sF(this.C, "scrollwheel", !0, n, !1, void 0), mK(this, 38, !1), mK(this, 40, !1), mK(this,
            37, !1), mK(this, 39, !1), mK(this, 32, !1), mK(this, 65, !1), mK(this, 68, !1), mK(this, 83, !1), mK(this, 87, !1), mK(this, 78, !1), mK(this, 85, !1), mK(this, 82, !1), mK(this, 97, !1), mK(this, 98, !1), mK(this, 99, !1), mK(this, 100, !1), mK(this, 101, !1), mK(this, 102, !1), mK(this, 103, !1), mK(this, 104, !1), mK(this, 105, !1), mK(this, 107, !0), mK(this, 109, !0), mK(this, 49, !1), mK(this, 50, !1), mK(this, 51, !1), mK(this, 52, !1), mK(this, 53, !1), mK(this, 54, !1), mK(this, 55, !1), mK(this, 56, !1), mK(this, 57, !1), mK(this, 187, !0), mK(this, 189, !0), nK(this, 91, !0), nK(this,
            17, !0), nK(this, 38, !1), nK(this, 40, !1), nK(this, 37, !1), nK(this, 39, !1), nK(this, 65, !1), nK(this, 68, !1), nK(this, 83, !1), nK(this, 87, !1));
        qF(this.C, "resize", !0, E(this.A.Yk, this.A));
        this.Y.wc(this.U, this.N, "contextmenu", null, yJ);
        this.K.F = this.A;
        n = this.I;
        d = new YJ(n.C, n, 0, new oK(this.A), void 0);
        n.A && RJ(d);
        this.ha = d;
        n = this.I;
        d = new cK(n.C, n, 0, new pK(this.A), void 0);
        n.A && RJ(d);
        this.Ma = d;
        n = this.I;
        d = new ZJ(n.C, n, n.D, 0, y(void 0) ? void 0 : 0, new qK(this.A), void 0);
        n.A && RJ(d);
        this.Ga = d;
        n = new rK(this.A);
        this.na = eK(this.I,
            n, 0);
        this.wa = eK(this.I, n, 1)
    }
    H(kK, GC);

    function mK(a, b, c) {
        c ? (c = E(a.A.Lg, a.A), sF(a.C, "keydown", !1, c, !0, b)) : (c = E(a.A.Lg, a.A), sF(a.C, "keydown", !1, c, !1, b))
    }

    function nK(a, b, c) {
        c ? (c = E(a.A.Mg, a.A), sF(a.C, "keyup", !1, c, !0, b)) : (c = E(a.A.Mg, a.A), sF(a.C, "keyup", !1, c, !1, b))
    }
    kK.prototype.ea = function(a) {
        this.Y.ra(a);
        this.A.ra(a);
        this.ha.ra(a);
        this.Ma.ra(a);
        this.Ga.ra(a);
        this.na.ra(a);
        this.wa.ra(a)
    };

    function oK(a) {
        this.A = a
    }
    oK.prototype.B = C;

    function pK(a) {
        this.B = a
    }
    pK.prototype.C = C;
    pK.prototype.A = C;

    function qK(a) {
        this.A = a
    }

    function rK(a) {
        this.A = a
    }
    kK.prototype.ga = function(a) {
        this.C.D("resize", null, a, null)
    };
    kK.prototype.Ua = function() {
        var a = lK(this);
        this.D.data[5] = a;
        dE(this.G)
    };

    function lK(a) {
        return (a = a.B.get()) && !xx(a) ? "Map" : a && 2 === Xs(a) ? "Photo" : a && 1 === Xs(a) ? "Street View" : "Main Display"
    };

    function sK(a, b, c, d, e, f, g, k, l, m, n, p, r, u) {
        V.call(this, "SCW", wb(arguments))
    }
    H(sK, V);

    function tK(a) {
        this.B = a;
        this.A = !1
    }

    function BE(a, b) {
        a.A ? b(null) : (a.A = !0, b(new uK(a.B)))
    }

    function uK(a) {
        this.A = a;
        (new Ws(P(this.A.A, 2))).data[0] = !1
    }
    uK.prototype.D = ca(0);
    uK.prototype.G = function() {
        return this.A.B
    };
    uK.prototype.H = function() {
        return this.A.A
    };

    function vK() {}
    vK.prototype.load = function(a, b, c) {
        b.ua("tdfl0");
        c(new tK(a), b);
        b.ua("tdfl1");
        return new DC
    };

    function wK(a, b, c, d, e, f, g, k, l, m, n, p, r, u, t, v) {
        m.getContext(function(b, n) {
            if (3 != L(d, 20, 1) && !b.Sa && !b.yb) {
                if (v) {
                    v(n);
                    return
                }
                throw Error("Could not build a rendering context for the scene.");
            }
            var x = new vK;
            b = new kK(c, d, m, b, e, new PD(f, c), g, k, l, 0, n, p, r, u, t, x);
            a(b)
        }, b)
    };

    function xK(a) {
        dA.call(this, a, "const float f=3.1415926;varying vec3 a;uniform vec4 b;attribute vec3 c;attribute vec2 d;uniform mat4 e;void main(){vec4 g=vec4(c,1);gl_Position=e*g;a=vec3(d.xy*b.xy+b.zw,1);a*=length(c);}", "precision highp float;const float h=3.1415926;varying vec3 a;uniform vec4 b;uniform float f;uniform sampler2D g;void main(){vec4 i=vec4(texture2DProj(g,a).rgb,f);gl_FragColor=i;}");
        this.B = new yK(this);
        this.attributes = new zK(this)
    }
    H(xK, dA);

    function yK(a) {
        this.B = new mA("b", a);
        this.D = new oA("e", a);
        this.A = new jA("f", a);
        this.F = new iA("g", a)
    }

    function zK(a) {
        this.B = new $z("c", a);
        this.A = new $z("d", a)
    };

    function AK(a) {
        dA.call(this, a, "attribute vec3 a;attribute vec2 b;uniform mat4 c;varying vec3 d;void main(){gl_Position=c*vec4(a,1);d=vec3(b.xy,1);}", "precision mediump float;uniform float e;uniform sampler2D f;varying vec3 d;void main(){vec4 g=texture2DProj(f,d);gl_FragColor=vec4(g.rgb,g.a*e);}");
        this.B = new BK(this);
        this.attributes = new CK(this)
    }
    H(AK, dA);

    function BK(a) {
        this.A = new oA("c", a);
        this.opacity = new jA("e", a);
        this.B = new iA("f", a)
    }

    function CK(a) {
        this.A = new $z("a", a);
        this.B = new $z("b", a)
    };

    function DK(a, b) {
        if (!b) return EK(a);
        try {
            FK(GK(a, 3553), b, 6408, 5121, 0)
        } catch (c) {
            return w.console && w.console.log(c), EK(a)
        }
        return b.width * b.height * 4
    }

    function EK(a) {
        HK(a, 3553, 0, 1, 1, 6408, 5121, new Uint8Array([0, 0, 0, 0]));
        return 4
    };

    function IK(a, b) {
        this.G = a;
        this.A = b;
        this.B = this.C = this.D = -1
    }
    IK.prototype.Ba = function() {
        return this.A.B.contains(this.D) && this.A.B.contains(this.C) && this.A.B.contains(this.B)
    };

    function JK(a, b) {
        return a.A.B.contains(b) ? (Yz(a.A.B, b), !0) : !1
    }
    IK.prototype.F = function() {
        if (!JK(this, this.D)) {
            var a = this.G.A,
                b = this.A.createBuffer(),
                c = this.A.B;
            this.D = c.B.add(b, c.O, 4 * a.length, 0);
            this.A.bindBuffer(34962, b);
            this.A.bufferData(34962, a, 35044)
        }
        JK(this, this.C) || (a = this.G, a = [0, a.D, 0, 0, a.F, a.D, a.F, 0], b = this.A.createBuffer(), c = this.A.B, this.C = c.B.add(b, c.O, 4 * a.length, 0), this.A.bindBuffer(34962, b), this.A.bufferData(34962, new Float32Array(a), 35044));
        JK(this, this.B) || (a = this.A.createTexture(), this.A.bindTexture(3553, a), this.A.texParameteri(3553, 10241, 9985),
            this.A.texParameteri(3553, 10240, 9729), this.A.texParameteri(3553, 10242, 33071), this.A.texParameteri(3553, 10243, 33071), b = DK(this.A, this.G.Aa()), b = Math.round(4 * b / 3), this.A.generateMipmap(3553), this.B = KK(this.A.B, a, b))
    };
    IK.prototype.Aa = function() {
        Yz(this.A.B, this.B);
        return this.A.B.Aa(this.B)
    };

    function LK(a, b) {
        om.call(this, a);
        this.A = b;
        this.C = new AK(b);
        this.F = null;
        this.J = 1
    }
    H(LK, om);
    LK.prototype.Md = function() {
        var a = this.A;
        a.depthMask(!1);
        a.disable(2884);
        a.enable(3042);
        a.disable(2929);
        a.disable(2960);
        a.disable(3089);
        LK.V.Md.call(this);
        a.depthMask(!0)
    };
    LK.prototype.I = function(a) {
        return new IK(a, this.A)
    };
    LK.prototype.B = function(a) {
        if (a.Ba() && this.F) {
            var b = this.A;
            Yz(a.A.B, a.D);
            var c = a.A.B.B.get(a.D) || null;
            Yz(a.A.B, a.C);
            var d = a.A.B.B.get(a.C) || null,
                e = this.A,
                f = this.C;
            fA(f);
            f.B.B.set(0);
            f.B.opacity.set(this.J);
            var g = aA(f.attributes.A),
                k = aA(f.attributes.B);
            e.enableVertexAttribArray(g);
            e.enableVertexAttribArray(k);
            e.bindBuffer(34962, c);
            f.attributes.A.vertexAttribPointer(3, 5126, !1, 0, 0);
            e.bindBuffer(34962, d);
            f.attributes.B.vertexAttribPointer(2, 5126, !1, 0, 0);
            pA(this.C.B.A, xm(this.F));
            b.bindBuffer(34962, c);
            b.activeTexture(33984);
            b.bindTexture(3553, a.Aa() || null);
            b.drawArrays(5, 0, 4);
            b.disableVertexAttribArray(aA(this.C.attributes.A));
            b.disableVertexAttribArray(aA(this.C.attributes.B))
        }
    };

    function MK(a, b, c, d, e, f) {
        gn.call(this, a, b, c, d, e);
        this.A = f;
        this.F = -1
    }
    H(MK, gn);
    MK.prototype.Aa = function() {
        Yz(this.A.B, this.F);
        return this.A.B.Aa(this.F) || null
    };
    MK.prototype.Ba = function() {
        return this.A.B.contains(this.F)
    };
    MK.prototype.G = function() {
        if (this.Ba()) Yz(this.A.B, this.F);
        else {
            var a = this.A.createTexture();
            this.A.bindTexture(3553, a);
            this.A.texParameteri(3553, 10241, 9729);
            this.A.texParameteri(3553, 10240, 9729);
            this.A.texParameteri(3553, 10242, 33071);
            this.A.texParameteri(3553, 10243, 33071);
            var b = DK(this.A, this.Ja());
            this.F = KK(this.A.B, a, b)
        }
    };

    function NK(a, b, c, d, e, f) {
        on.call(this, a, b, c, d, e);
        this.A = f;
        this.I = -1;
        this.M = this.N = this.O = null
    }
    H(NK, on);
    NK.prototype.Eb = function() {
        return this.A.B.contains(this.I)
    };
    NK.prototype.ce = function() {
        this.A.B.contains(this.I) ? Yz(this.A.B, this.I) : OK(this)
    };

    function OK(a) {
        var b = vn(a);
        a.N = a.A.createBuffer();
        var c = Tm(a);
        a.M = a.A.createBuffer();
        var d = b.byteLength + c.byteLength,
            e = a.D instanceof bq,
            f = null;
        e && (f = Um(a), a.O = a.A.createBuffer(), d += f.byteLength);
        a.I = Wz(a.A.B, function() {
            e && a.O && a.A.deleteBuffer(a.O);
            a.N && a.A.deleteBuffer(a.N);
            a.M && a.A.deleteBuffer(a.M);
            a.O = a.M = a.N = null;
            a.I = -1
        }, d);
        e && (a.A.bindBuffer(34962, a.O), a.A.bufferData(34962, f, 35044));
        a.A.bindBuffer(34962, a.N);
        a.A.bufferData(34962, b, 35044);
        a.A.bindBuffer(34963, a.M);
        a.A.bufferData(34963, c, 35044)
    }

    function PK(a) {
        Yz(a.A.B, a.I);
        return a.O
    }

    function QK(a) {
        Yz(a.A.B, a.I);
        return a.N
    }

    function RK(a) {
        Yz(a.A.B, a.I);
        return a.M
    }
    NK.prototype.Ae = function(a, b) {
        return new MK(a.C, a.D, a.B, a.Ja(), b, this.A)
    };

    function SK(a) {
        this.A = a
    }
    SK.prototype.create = function(a, b, c, d, e) {
        return new NK(a, b, c, d, e, this.A)
    };

    function TK(a) {
        dA.call(this, a, "attribute vec2 a;uniform vec4 b;uniform mat4 c;varying vec2 d;void main(){gl_Position=c*vec4(a.x,a.y,1,1);d=a.xy*b.xy+b.zw;}", "precision highp float;uniform float e,f;uniform sampler2D g;varying vec2 d;float j(){if(f==0.)return 1.;else{vec2 h=abs(d-.5)-.5+f;return 1.-length(max(h,0.))/f;}}void main(){vec4 h=texture2D(g,d);float i=j();gl_FragColor=vec4(h.rgb,e*i);}");
        this.B = new UK(this);
        this.attributes = new VK(this)
    }
    H(TK, dA);

    function UK(a) {
        this.B = new mA("b", a);
        this.D = new oA("c", a);
        this.A = new jA("e", a);
        this.C = new jA("f", a);
        this.F = new iA("g", a)
    }

    function VK(a) {
        this.A = new $z("a", a)
    };

    function WK(a) {
        this.A = a;
        this.D = new xK(this.A);
        this.F = new TK(this.A);
        this.C = new LK(new zm, this.A);
        this.B = 0
    }
    WK.prototype.H = function() {
        this.A.bindFramebuffer(36160, null);
        this.A.cullFace(1029);
        this.A.depthFunc(515);
        this.A.depthMask(!0);
        this.A.disable(3089);
        this.A.disable(2960);
        for (var a = 0; 8 > a; ++a) this.A.disableVertexAttribArray(a);
        this.A.enable(3042);
        this.A.enable(2884);
        this.A.enable(2929);
        this.A.blendFuncSeparate(770, 771, 1, 771);
        a = this.A.C.A;
        this.A.viewport(0, 0, a.width, a.height);
        this.A.clearColor(this.B, this.B, this.B, 1);
        this.A.clear(16640)
    };
    WK.prototype.I = function(a, b, c) {
        if (0 != N(b, 0)) {
            this.A.clear(256);
            var d = a instanceof bq,
                e = d ? this.D : this.F;
            fA(e);
            pA(e.B.D, xm(c));
            e.B.A.set(N(b, 0));
            e.B.F.set(0);
            var f = -1;
            d && (f = aA(e.attributes.B), this.A.enableVertexAttribArray(f));
            var g = aA(e.attributes.A);
            this.A.enableVertexAttribArray(g);
            e.B.C && e.B.C.set(N(b, 1));
            var k = Sm(c);
            this.A.activeTexture(33984);
            for (var l = 0, m = k.length; l < m; ++l) {
                var n, p = a,
                    r = k[l],
                    u = b,
                    t = e;
                if (r.Ba()) {
                    p instanceof bq && (this.A.bindBuffer(34962, PK(r)), t.attributes.B.vertexAttribPointer(3,
                        5126, !1, 0, 0));
                    this.A.bindBuffer(34962, QK(r));
                    t.attributes.A.vertexAttribPointer(2, 5126, !1, 0, 0);
                    this.A.bindBuffer(34963, RK(r));
                    var p = Tm(r),
                        u = N(u, 0),
                        v = r.P;
                    r.X || r.Sc();
                    n = r.X;
                    if (1 > v && n) {
                        var v = Vx(v),
                            x = n.Aa();
                        this.A.bindTexture(3553, x);
                        nA(t.B.B, n.H);
                        t.B.A.set(u);
                        this.A.drawElements(5, p.length, 5123, 0)
                    }
                    r = r.Sc();
                    n = r.Aa();
                    this.A.bindTexture(3553, n);
                    nA(t.B.B, r.H);
                    t.B.A.set(u * v);
                    this.A.drawElements(5, p.length, 5123, 0)
                }
            }
            e.B.A.set(N(b, 0));
            this.A.disableVertexAttribArray(g);
            d && this.A.disableVertexAttribArray(f);
            b = N(b, 4);
            a = a.Nb();
            0 < b && 0 < a.length && (this.C.hc(a), this.C.F = c, this.C.J = b, this.C.Pa())
        }
    };
    WK.prototype.J = ba("B");

    function XK(a, b, c) {
        var d = new WK(b);
        ep.call(this, a, d, new tq(new SK(b)), c)
    }
    H(XK, Lp);

    function YK(a, b, c, d, e) {
        b = new XK(c, d, e);
        a(b)
    };

    function ZK() {};

    function $K() {
        return Yb("iPad") || Yb("Android") && !Yb("Mobile") || Yb("Silk")
    };

    function aL(a, b) {
        var c;
        document.createEvent ? (c = document.createEvent("Event"), c.initEvent(b || a.type, !0, !0)) : (c = document.createEventObject(), c.type = b || a.type);
        c.ad = a.timeStamp;
        return c
    };

    function bL() {
        this.F = [];
        this.A = [];
        this.B = [];
        this.G = {};
        this.C = null;
        this.D = []
    }
    var cL = "undefined" != typeof navigator && /iPhone|iPad|iPod/.test(navigator.userAgent),
        dL = String.prototype.trim ? function(a) {
            return a.trim()
        } : function(a) {
            return a.replace(/^\s+/, "").replace(/\s+$/, "")
        },
        eL = /\s*;\s*/;

    function fL(a, b) {
        return function(c) {
            var d;
            d = b;
            var e;
            "click" == d && (fo && c.metaKey || !fo && c.ctrlKey || 2 == c.which || null == c.which && 4 == c.button || c.shiftKey) && (d = "clickmod");
            var f = c.srcElement || c.target,
                g = gL(d, c, f, "", null),
                k, l;
            for (e = f; e && e != this; e = e.__owner || e.parentNode) {
                k = l = e;
                var m = d,
                    n = k.__jsaction;
                if (!n) {
                    var p = hL(k, "jsaction");
                    if (p) {
                        n = qH[p];
                        if (!n) {
                            for (var n = {}, r = p.split(eL), u = 0, t = r ? r.length : 0; u < t; u++) {
                                var v = r[u];
                                if (v) {
                                    var x = v.indexOf(":"),
                                        D = -1 != x,
                                        z = D ? dL(v.substr(0, x)) : "click",
                                        v = D ? dL(v.substr(x + 1)) : v;
                                    n[z] =
                                        v
                                }
                            }
                            qH[p] = n
                        }
                        p = n;
                        n = {};
                        for (z in p) {
                            r = n;
                            u = z;
                            b: if (t = p[z], !(0 <= t.indexOf(".")))
                                    for (v = k; v; v = v.parentNode) {
                                        x = v;
                                        D = x.__jsnamespace;
                                        y(D) || (D = hL(x, "jsnamespace"), x.__jsnamespace = D);
                                        if (x = D) {
                                            t = x + "." + t;
                                            break b
                                        }
                                        if (v == this) break
                                    }
                                r[u] = t
                        }
                        k.__jsaction = n
                    } else n = iL, k.__jsaction = n
                }
                k = {
                    rb: m,
                    action: n[m] || "",
                    event: null,
                    dk: !1
                };
                if (k.dk || k.action) break
            }
            k && (g = gL(k.rb, k.event || c, f, k.action || "", l, g.timeStamp));
            g && "touchend" == g.eventType && (g.event._preventMouseEvents = jo);
            k && k.action || (g.action = "", g.actionElement = null);
            d = g;
            a.C && (e =
                gL(d.eventType, d.event, d.targetElement, d.action, d.actionElement, d.timeStamp), "clickonly" == e.eventType && (e.eventType = "click"), a.C(e, !0));
            if (d.actionElement && ("A" != d.actionElement.tagName || "click" != d.eventType && "clickmod" != d.eventType || co(c), a.C ? a.C(d) : (c = ko(c), d.event = c, a.D.push(d)), "touchend" == d.event.type && d.event._mouseEventsPrevented)) {
                c = d.event;
                for (var A in c) d = c[A], "type" == A || "srcElement" == A || ya(d);
                F()
            }
        }
    }

    function gL(a, b, c, d, e, f) {
        return {
            eventType: a,
            event: b,
            targetElement: c,
            action: d,
            actionElement: e,
            timeStamp: f || F()
        }
    }

    function hL(a, b) {
        var c = null;
        "getAttribute" in a && (c = a.getAttribute(b));
        return c
    }
    var iL = {};

    function jL(a, b) {
        return function(c) {
            var d = a,
                e = b,
                f = !1;
            "mouseenter" == d ? d = "mouseover" : "mouseleave" == d && (d = "mouseout");
            if (c.addEventListener) {
                if ("focus" == d || "blur" == d || "error" == d || "load" == d) f = !0;
                c.addEventListener(d, e, f)
            } else c.attachEvent && ("focus" == d ? d = "focusin" : "blur" == d && (d = "focusout"), e = bo(c, e), c.attachEvent("on" + d, e));
            return {
                rb: d,
                gb: e,
                capture: f
            }
        }
    }
    bL.prototype.gb = function(a) {
        return this.G[a]
    };

    function RD(a) {
        for (var b = a.B.concat(a.A), c = [], d = [], e = 0; e < a.A.length; ++e) {
            var f = a.A[e];
            kL(f, b) ? (c.push(f), QD(f)) : d.push(f)
        }
        for (e = 0; e < a.B.length; ++e) f = a.B[e], kL(f, b) ? c.push(f) : (d.push(f), UD(a, f));
        a.A = d;
        a.B = c
    }

    function UD(a, b) {
        var c = b.A;
        cL && (c.style.cursor = "pointer");
        for (c = 0; c < a.F.length; ++c) b.B.push(a.F[c].call(null, b.A))
    }

    function SD(a) {
        this.A = a;
        this.B = []
    }

    function kL(a, b) {
        for (var c = 0; c < b.length; ++c)
            if (b[c].A != a.A && TD(b[c].A, a.A)) return !0;
        return !1
    }

    function TD(a, b) {
        for (; a != b && b.parentNode;) b = b.parentNode;
        return a == b
    }

    function QD(a) {
        for (var b = 0; b < a.B.length; ++b) {
            var c = a.A,
                d = a.B[b];
            c.removeEventListener ? c.removeEventListener(d.rb, d.gb, d.capture) : c.detachEvent && c.detachEvent("on" + d.rb, d.gb)
        }
        a.B = []
    };

    function lL(a, b) {
        this.C = a;
        this.B = b;
        this.A = {};
        this.A.hashchange = "hashchange";
        this.A.resize = "resize";
        this.A.load = "load";
        this.A.unload = "unload";
        this.A.beforeunload = "beforeunload";
        a = document;
        (a = "hidden" in a ? "visibilitychange" : "mozHidden" in a ? "mozvisibilitychange" : "msHidden" in a ? "msvisibilitychange" : "webkitHidden" in a ? "webkitvisibilitychange" : "") && (this.A[a] = "visibilitychange")
    }
    lL.prototype.cb = function() {
        var a = {
            popstate: ["popstate"],
            error: ["error"]
        };
        Mb(this.A, function(b, c) {
            a[b] || (a[b] = []);
            a[b].push(c)
        });
        return a
    };
    lL.prototype.Na = ca(null);
    lL.prototype.nb = function(a) {
        var b = a.type,
            c = new wo(this.C, b);
        if ("popstate" == b) this.B("popstate", c, a);
        else if ("error" == b) {
            var d, e, f = unescape(a.message),
                b = f.split("~#!#~");
            4 == b.length ? (e = oj[parseInt(b[1], 10)], d = e.$f, e = e.error, b = b[0] + b[2] + b[3]) : b = f;
            a.message = b;
            a.file = a.file;
            a.ik = parseInt(a.line, 10);
            a.stack = a.stack;
            a.Cn = a.stackUrls;
            a.Bn = a.stackTruncation;
            a.$f = a.errorType;
            d && (a.$f = d);
            a.wn = a.count;
            a.error = e;
            this.B("error", c, a)
        } else this.A[b] && (d = this.A[b], "visibilitychange" == d ? (e = document, b = !1, "hidden" in
            e ? b = e.hidden : "mozHidden" in e ? b = e.mozHidden : "msHidden" in e ? b = e.msHidden : "webkitHidden" in e && (b = e.webkitHidden), a.hidden = b, this.B(d, c, a)) : this.B(d, c));
        c.done("main-actionflow-branch")
    };
    lL.prototype.mb = C;

    function mL(a) {
        this.A = a
    }
    mL.prototype.cb = ca(null);
    mL.prototype.Na = function() {
        return {
            copy: {
                qa: ["copy"],
                global: null
            },
            cut: {
                qa: ["cut"],
                global: null
            }
        }
    };
    mL.prototype.nb = C;
    mL.prototype.mb = function(a, b) {
        b.event();
        var c = b.event().type;
        "copy" == c ? (c = b.event(), this.A(a, "copy", b, c)) : "cut" == c && (c = b.event(), this.A(a, "cut", b, c))
    };

    function nL() {
        this.A = {}
    }

    function oL(a, b, c, d) {
        b = Aa(b);
        d = d ? 1 : -1;
        for (var e = 0, f = c.length; e < f; ++e) {
            var g = c[e],
                g = a.A[g] = a.A[g] || new $i,
                k = g.get(b, 0) + d;
            g.set(b, k)
        }
    }

    function pL(a, b, c) {
        b = Aa(b);
        return !!a.A[c] && 0 < a.A[c].get(b, 0)
    }

    function qL(a) {
        var b = [],
            c;
        for (c in a.A) {
            var d = a.A[c];
            d && pb(d.Ca(), function(a) {
                return 0 < a
            }) && b.push(c)
        }
        return new fj(b)
    };

    function rL(a, b) {
        this.B = {};
        this.G = {};
        this.K = {};
        this.C = [];
        this.I = a || sL;
        this.F = b;
        this.A = {};
        this.D = null
    }
    rL.prototype.H = function(a, b) {
        if (ua(a)) this.C = wb(a), tL(this);
        else if (b) {
            b = a.event;
            if (a = this.A[a.eventType])
                for (var c = !1, d = 0, e; e = a[d++];) !1 === e(b) && (c = !0);
            c && co(b)
        } else d = a.action, c = d.split(".")[0], b = this.G[c], this.F ? e = this.F(a) : b ? b.accept(a) && (e = b.handle) : e = this.B[d], e ? (a = this.I(a), e(a), a.done("main-actionflow-branch")) : (e = ko(a.event), a.event = e, this.C.push(a), b || (e = this.K[c]) && !e.wi && (e.un(this, c, a), e.wi = !0))
    };

    function sL(a) {
        return new lo(a.action, a.actionElement, a.event, a.timeStamp, a.eventType)
    }

    function uL(a, b, c) {
        Mb(c, E(function(a, c) {
            a = b ? E(a, b) : a;
            this.B[c] = a
        }, a));
        tL(a)
    }

    function vL(a, b) {
        return a.B.hasOwnProperty(b) || a.G.hasOwnProperty(b.split(".")[0])
    }

    function tL(a) {
        a.D && 0 != a.C.length && Kq(function() {
            this.D(this.C, this)
        }, a)
    };

    function wL(a, b, c) {
        this.C = null;
        this.B = a;
        a = 0;
        for (var d = this.B.length; a < d; ++a) {
            var e = this.B[a].Na();
            if (e)
                for (var f in e)
                    for (var g = e[f].qa, k = 0, l = g.length; k < l; ++k) {
                        var m = b,
                            n = g[k];
                        if (!m.G.hasOwnProperty(n) && "mouseenter" != n && "mouseleave" != n) {
                            var p = fL(m, n),
                                r = jL(n, p);
                            m.G[n] = p;
                            m.F.push(r);
                            for (n = 0; n < m.A.length; ++n) p = m.A[n], p.B.push(r.call(null, p.A))
                        }
                    }
        }
        c = this.A = new rL(xL(c));
        c.D = yL;
        tL(c);
        c = E(this.A.H, this.A);
        b.C = c;
        b.D && (0 < b.D.length && c(b.D), b.D = null);
        this.C = E(this.F, this);
        b = 0;
        for (c = zL.length; b < c; b++) f = this.A,
            a = zL[b], d = this.C, f.A[a] = f.A[a] || [], f.A[a].push(d);
        this.D = {}
    }

    function yL(a, b) {
        if (0 != a.length) {
            var c = a[a.length - 1];
            if (vL(b, c.action)) {
                b = c.event;
                var d = c.eventType,
                    e;
                "_custom" == b.type ? e = "_custom" : e = d || b.type;
                if ("keypress" == e || "keydown" == e || "keyup" == e) {
                    var f;
                    if (ho) f = aL(b, d), f.ctrlKey = b.ctrlKey, f.altKey = b.altKey, f.shiftKey = b.shiftKey, f.metaKey = b.metaKey, f.keyCode = b.keyCode, f.charCode = b.charCode, f.ad = b.timeStamp, d = f;
                    else {
                        if (document.createEvent)
                            if (f = document.createEvent("KeyboardEvent"), f.initKeyboardEvent) {
                                e = b.ctrlKey;
                                var g = b.metaKey,
                                    k = b.shiftKey,
                                    l = [];
                                b.altKey &&
                                    l.push("Alt");
                                e && l.push("Control");
                                g && l.push("Meta");
                                k && l.push("Shift");
                                f.initKeyboardEvent(d || b.type, !0, !0, window, b.charCode, b.keyCode, b.location, l.join(" "), b.repeat, b.locale);
                                if (go || io) d = Ti(b.keyCode), Object.defineProperty(f, "keyCode", {
                                    get: d
                                }), Object.defineProperty(f, "which", {
                                    get: d
                                })
                            } else f.initKeyEvent(d || b.type, !0, !0, window, b.ctrlKey, b.altKey, b.shiftKey, b.metaKey, b.keyCode, b.charCode);
                        else f = document.createEventObject(), f.type = d || b.type, f.repeat = b.repeat, f.ctrlKey = b.ctrlKey, f.altKey = b.altKey,
                            f.shiftKey = b.shiftKey, f.metaKey = b.metaKey, f.keyCode = b.keyCode, f.charCode = b.charCode;
                        f.ad = b.timeStamp;
                        d = f
                    }
                } else if ("click" == e || "dblclick" == e || "mousedown" == e || "mouseover" == e || "mouseout" == e || "mousemove" == e) document.createEvent ? (f = document.createEvent("MouseEvent"), f.initMouseEvent(d || b.type, !0, !0, window, b.detail || 1, b.screenX || 0, b.screenY || 0, b.clientX || 0, b.clientY || 0, b.ctrlKey || !1, b.altKey || !1, b.shiftKey || !1, b.metaKey || !1, b.button || 0, b.relatedTarget || null)) : (f = document.createEventObject(), f.type = d || b.type,
                    f.clientX = b.clientX, f.clientY = b.clientY, f.button = b.button, f.detail = b.detail, f.ctrlKey = b.ctrlKey, f.altKey = b.altKey, f.shiftKey = b.shiftKey, f.metaKey = b.metaKey), f.ad = b.timeStamp, d = f;
                else if ("focus" == e || "blur" == e || "focusin" == e || "focusout" == e || "scroll" == e) document.createEvent ? (f = document.createEvent("UIEvent"), f.initUIEvent(d || b.type, y(b.bubbles) ? b.bubbles : !0, b.cancelable || !1, b.view || window, b.detail || 0)) : (f = document.createEventObject(), f.type = d || b.type, f.bubbles = y(b.bubbles) ? b.bubbles : !0, f.cancelable = b.cancelable ||
                    !1, f.view = b.view || window, f.detail = b.detail || 0), f.relatedTarget = b.relatedTarget || null, f.ad = b.timeStamp, d = f;
                else if ("_custom" == e) {
                    d = {
                        _type: d,
                        type: d,
                        data: b.detail.data,
                        Dn: b.detail.triggeringEvent
                    };
                    try {
                        f = document.createEvent("CustomEvent"), f.initCustomEvent("_custom", !0, !1, d)
                    } catch (m) {
                        f = document.createEvent("HTMLEvents"), f.initEvent("_custom", !0, !1), f.detail = d
                    }
                    d = f;
                    d.ad = b.timeStamp
                } else d = aL(b, d);
                c = c.targetElement;
                b = d;
                c.dispatchEvent ? c.dispatchEvent(b) : c.fireEvent("on" + b.type, b);
                a.length = 0
            }
        }
    }
    var zL = "click rightclick contextmenu mousedown keypress wheel".split(" ");

    function xL(a) {
        return function(b) {
            return new wo(a, b.action, b.actionElement, b.event)
        }
    }
    wL.prototype.F = function(a) {
        var b = w.globals && w.globals.fua;
        b && !y(b.data) && (b.data = {
            type: a.type,
            target: a.target,
            currentTarget: a.currentTarget,
            time: F(),
            jn: !1
        }, b.dispose && b.dispose());
        if (this.C) {
            a = 0;
            for (b = zL.length; a < b; a++) {
                var c = this.A,
                    d = zL[a];
                c.A[d] && tb(c.A[d], this.C)
            }
            this.C = null
        }
    };
    wL.prototype.H = function(a, b, c) {
        if ("" != a) {
            var d = this.D[a];
            d || (d = this.D[a] = new nL);
            for (var e = 0, f = this.B.length; e < f; ++e) {
                var g = this.B[e],
                    k = g.Na();
                k && (k = k[b]) && k.qa && oL(d, g, k.qa, c)
            }
            qL(d).Oa() ? delete this.A.B[a] : vL(this.A, a) || (b = {}, b[a] = this.G, uL(this.A, this, b))
        }
    };
    wL.prototype.G = function(a) {
        try {
            for (var b = a.P, c = a.event().type, d = this.D[b], e = 0, f = this.B.length; e < f; ++e) {
                var g = this.B[e];
                pL(d, g, c) && g.mb(b, a)
            }
        } catch (k) {
            throw pj(k);
        }
    };

    function AL(a) {
        this.A = a
    }
    AL.prototype.cb = ca(null);
    AL.prototype.Na = function() {
        return {
            focus: {
                qa: ["focus"],
                global: null
            },
            blur: {
                qa: ["blur"],
                global: null
            }
        }
    };
    AL.prototype.nb = C;
    AL.prototype.mb = function(a, b) {
        b.event();
        var c = b.event().type;
        "focus" == c ? (c = b.event(), this.A(a, "focus", b, c)) : "blur" == c && (c = b.event(), this.A(a, "blur", b, c))
    };

    function BL(a) {
        this.A = a
    }
    BL.prototype.cb = ca(null);
    BL.prototype.Na = function() {
        return {
            change: {
                qa: ["change"],
                global: null
            },
            input: {
                qa: ["input"],
                global: null
            }
        }
    };
    BL.prototype.nb = C;
    BL.prototype.mb = function(a, b) {
        b.event();
        var c = b.event().type;
        "change" == c ? (c = b.event(), this.A(a, "change", b, c)) : "input" == c && (c = b.event(), this.A(a, "input", b, c))
    };

    function CL(a, b) {
        wq.call(this);
        this.B = a;
        this.J = b || window;
        this.D = new nL;
        this.C = new nL;
        "undefined" !== typeof globals && globals.ErrorHandler && globals.ErrorHandler.listen ? (globals.ErrorHandler.listen(E(this.G, this)), a = !0) : a = !1;
        this.I = a
    }
    H(CL, wq);
    CL.prototype.H = function(a, b, c) {
        var d = "" == a,
            e = d ? this.C : this.D,
            f = d ? this.D : this.C;
        a = qL(f);
        for (var g = 0, k = this.B.length; g < k; ++g) {
            var l = this.B[g],
                m;
            var n = l;
            m = b;
            m = d ? (n = n.cb()) ? n[m] : void 0 : (n = n.Na()) && n[m] ? n[m].global : void 0;
            m && oL(f, l, m, c)
        }
        b = qL(f);
        e = qL(e);
        c = ij(ij(a, b), e);
        a = ij(ij(b, a), e);
        nb(c.Ca(), this.N, this);
        nb(a.Ca(), this.L, this)
    };
    CL.prototype.L = function(a) {
        "error" == a && this.I || yq(this, this.J, a, this.G, !0, this)
    };
    CL.prototype.N = function(a) {
        "error" == a && this.I || this.Ub(this.J, a, this.G, !0, this)
    };
    CL.prototype.G = function(a) {
        try {
            for (var b = a.type, c = 0, d = this.B.length; c < d; ++c) {
                var e = this.B[c];
                (pL(this.D, e, b) || pL(this.C, e, b)) && e.nb(a)
            }
        } catch (f) {
            throw globals.ErrorHandler.log(f);
        }
    };
    var DL = {},
        EL = {};

    function FL(a) {
        var b = Aa(a);
        DL[b] && (vo(a, "lhc", DL[b].toString()), vo(a, "lht", EL[b].toFixed(3).toString()), delete DL[b], delete EL[b])
    };

    function GL(a, b, c) {
        this.B = a;
        this.A = b;
        this.C = c
    }
    GL.prototype.cb = function() {
        return {
            keypress: ["keypress"],
            keydown: ["keydown"],
            keyup: ["keyup"]
        }
    };
    GL.prototype.Na = function() {
        return {
            keypress: {
                qa: ["keypress"],
                global: null
            },
            keydown: {
                qa: ["keydown"],
                global: null
            },
            keyup: {
                qa: ["keyup"],
                global: null
            }
        }
    };
    GL.prototype.nb = function(a) {
        var b = HL(a);
        if (b) {
            var c = new wo(this.B, b);
            a = IL(a);
            this.C(b, c, a, a.keyCode);
            c.done("main-actionflow-branch")
        }
    };
    GL.prototype.mb = function(a, b) {
        b.event();
        var c = b.event(),
            d = HL(c);
        d && (c = IL(c), this.A(a, d, b, c, c.keyCode))
    };

    function HL(a) {
        switch (a.type) {
            case "keypress":
                return "keypress";
            case "keydown":
                return "keydown";
            case "keyup":
                return "keyup";
            default:
                return null
        }
    }

    function IL(a) {
        return {
            type: a.type,
            keyCode: Zx(a.keyCode),
            shiftKey: a.shiftKey,
            ctrlKey: a.ctrlKey,
            altKey: a.altKey,
            metaKey: a.metaKey,
            yn: a,
            preventDefault: function() {
                a.preventDefault()
            },
            stopPropagation: function() {
                a.stopPropagation()
            }
        }
    };

    function JL() {
        this.A = KL()
    }

    function KL() {
        return y(window.performance) ? window.performance.now || window.performance.mozNow || window.performance.msNow || window.performance.oNow || window.performance.webkitNow || null : null
    }
    JL.prototype.getTime = function() {
        return this.A.call(window.performance)
    };

    function LL(a, b) {
        b || (b = eo(a), b = b.getBoundingClientRect || !b.parentNode ? b : b.parentNode, b = b.getBoundingClientRect());
        ML.x = a.clientX - b.left;
        ML.y = a.clientY - b.top;
        return ML
    }

    function NL(a) {
        a.getAttribute("tabindex") || a.setAttribute("tabindex", "-1");
        a.focus()
    }
    var ML = new Dj;

    function OL(a, b) {
        this.M = a;
        this.L = b;
        this.A = !1;
        this.B = null;
        this.C = !1;
        this.H = "";
        this.F = this.D = 0;
        this.G = this.J = null;
        this.K = this.I = 0
    }

    function PL(a, b, c, d) {
        a.C || (a.H = b, a.J = d, a.G = d.getBoundingClientRect(), b = LL(c, a.G), a.I = a.D = b.x, a.K = a.F = b.y, a.C = !0)
    }

    function QL(a, b, c) {
        if (a.C) {
            var d = RL(b) ? 15 : 2;
            c = LL(c, a.G);
            !a.A && (Math.abs(a.I - c.x) > d || Math.abs(a.K - c.y) > d) && (a.A = !0, a.B = new wo(a.M, a.H), SL(a, b, "dragstart", a.I, a.K));
            a.A && (SL(a, b, "drag", c.x, c.y), a.D = c.x, a.F = c.y)
        }
    }

    function TL(a, b, c) {
        if (!a.C) return !1;
        var d = a.D,
            e = a.F;
        c && (c = LL(c, a.G), d = c.x, e = c.y);
        a.A && (SL(a, b, "dragend", d, e), a.B && a.B.done("main-actionflow-branch"), a.B = null);
        b = a.A;
        a.C = !1;
        a.A = !1;
        return b
    }

    function SL(a, b, c, d, e) {
        var f = a.B;
        b.x = d;
        b.y = e;
        b.mn = d - a.D;
        b.nn = e - a.F;
        RL(b) || (b.target = a.J);
        a.L(a.H, c, f, b)
    }

    function RL(a) {
        return "touchstart" === a.type || "touchmove" === a.type || "touchend" === a.type || "touchcancel" === a.type
    };

    function UL(a, b) {
        this.D = b;
        this.B = !1;
        this.C = null;
        this.A = [];
        b = [0, 1, 2];
        for (var c = 0, d = b.length; c < d; ++c) this.A.push(new OL(a, E(this.pk, this, b[c])))
    }
    q = UL.prototype;
    q.cb = ca(null);
    q.Na = function() {
        var a;
        Gk ? a = {
            qa: ["mousedown"],
            global: ["mousemove", "mouseup"]
        } : a = {
            qa: ["mousedown", "mousemove", "mouseup"],
            global: null
        };
        var b = {};
        b.dragstart = a;
        b.drag = a;
        b.dragend = a;
        return b
    };
    q.nb = function(a) {
        VL(this, a)
    };
    q.mb = function(a, b) {
        b.event();
        var c = new Ik(b.event());
        VL(this, c, a, b)
    };

    function VL(a, b, c, d) {
        var e;
        if ("mousedown" == b.type) b = b.ab, NL(eo(b)), co(b), (e = a.A[b.button]) && PL(e, c, b, d.node());
        else if ("mousemove" == b.type)
            for (c = 0, d = a.A.length; c < d; ++c) QL(a.A[c], b, b);
        else "mouseup" == b.type && (e = a.A[b.button], a.B = !!e && TL(e, b, b), a.C = a.B ? b.target : null)
    }
    q.pk = function(a, b, c, d, e) {
        this.D(b, c, d, e, a)
    };

    function WL() {
        this.A = [];
        this.B = "touch";
        XL() && (this.B = w.MSPointerEvent.MSPOINTER_TYPE_TOUCH)
    }

    function XL() {
        return !y(w.PointerEvent) && y(w.MSPointerEvent)
    }

    function YL() {
        return y(w.PointerEvent) || y(w.MSPointerEvent)
    }

    function ZL(a) {
        this.identifier = a.pointerId;
        this.screenX = a.screenX;
        this.screenY = a.screenY;
        this.clientX = a.clientX;
        this.clientY = a.clientY;
        this.pageX = a.pageX;
        this.pageY = a.pageY;
        this.force = a.pressure;
        this.target = a.target
    }

    function $L(a, b) {
        var c = rb(a.A, function(a) {
                return a.identifier == b.pointerId
            }),
            d = new ZL(b); - 1 == c ? a.A.push(d) : a.A[c] = d;
        return d
    }

    function aM(a, b, c, d) {
        var e = {};
        e.type = b;
        e.touches = wb(a.A);
        e.changedTouches = [c];
        e.target = d.target;
        e.currentTarget = d.currentTarget;
        e.preventDefault = function() {
            d.preventDefault()
        };
        return e
    }

    function bM(a, b) {
        switch (b.type) {
            case "pointerdown":
            case "MSPointerDown":
                if (b.pointerType == a.B) {
                    y(b.target.F) ? b.target.F(b.pointerId) : y(b.target.msSetPointerCapture) && b.target.msSetPointerCapture(b.pointerId);
                    var c = $L(a, b);
                    a = aM(a, "touchstart", c, b)
                } else a = null;
                return a;
            case "pointermove":
            case "MSPointerMove":
                return b.pointerType == a.B ? (c = $L(a, b), a = aM(a, "touchmove", c, b)) : a = null, a;
            case "pointerup":
            case "pointercancel":
            case "MSPointerUp":
            case "MSPointerCancel":
                return cM(a, b)
        }
        return null
    }

    function cM(a, b) {
        if (b.pointerType == a.B) {
            y(b.target.D) ? b.target.D(b.pointerId) : y(b.target.msReleasePointerCapture) && b.target.msReleasePointerCapture(b.pointerId);
            var c = rb(a.A, function(a) {
                return a.identifier == b.pointerId
            });
            if (-1 != c) return ub(a.A, c), aM(a, "touchend", new ZL(b), b)
        }
        return null
    };

    function dM(a, b) {
        this.K = a;
        this.C = b;
        this.A = !1;
        this.F = (a = XL()) ? "MSPointerDown" : "pointerdown";
        this.G = a ? "MSPointerUp" : "pointerup";
        this.I = a ? "MSPointerCancel" : "pointercancel";
        this.H = a ? w.MSPointerEvent.MSPOINTER_TYPE_TOUCH : "touch";
        this.D = y(w.TouchEvent) && YL();
        this.B = !1
    }
    q = dM.prototype;
    q.cb = ca(null);
    q.Na = function() {
        var a = {
            click: {
                qa: ["click"],
                global: null
            },
            dblclick: {
                qa: ["dblclick"],
                global: null
            }
        };
        a.ptrdown = {
            qa: ["mousedown", "touchstart", this.F],
            global: null
        };
        a.ptrhover = {
            qa: ["mousemove"],
            global: ["mousedown", "mouseup"]
        };
        a.ptrup = {
            qa: ["mouseup", "touchend", this.G, this.I],
            global: null
        };
        a.contextmenu = {
            qa: ["contextmenu"],
            global: null
        };
        return a
    };
    q.nb = function(a) {
        a = a.type;
        "mousedown" == a ? this.A = !0 : "mouseup" == a && (this.A = !1)
    };

    function eM(a, b) {
        if ("mousedown" == b.type) return !0;
        if (a.D && a.B || "touchstart" != b.type) return b.type == a.F && b.pointerType == a.H ? (a.B = !0, b.isPrimary) : !1;
        a = b.touches;
        return 1 == (a ? a.length : 0)
    }
    q.mb = function(a, b) {
        b.event();
        var c = b.event(),
            d = c.type;
        "click" == d ? (d = b.event(), c = this.C.C, this.C.B && d && c && d.target === c ? b.event().stopPropagation() : fM(this, a, "click", b)) : "dblclick" == d ? fM(this, a, "dblclick", b) : eM(this, c) ? fM(this, a, "ptrdown", b) : "mousemove" != d || this.A ? ("mouseup" == c.type ? c = !0 : this.D && this.B || "touchend" != c.type ? c = c.type == this.G && c.pointerType == this.H ? c.isPrimary : !1 : (c = c.touches, c = 0 == (c ? c.length : 0)), c ? fM(this, a, "ptrup", b) : "contextmenu" == d && fM(this, a, "contextmenu", b)) : fM(this, a, "ptrhover",
            b)
    };
    q.Wd = h("A");

    function fM(a, b, c, d) {
        var e = d.node();
        if (e) {
            var f = d.event(),
                g = new Ik(f);
            if ("touchstart" == f.type || "touchend" == f.type) {
                var k = f.touches;
                0 == k.length && (k = f.changedTouches);
                f = k[0];
                g.clientX = f.clientX;
                g.clientY = f.clientY;
                g.screenX = f.screenX;
                g.screenY = f.screenY
            }
            e = LL(g, e.getBoundingClientRect());
            g.x = e.x;
            g.y = e.y;
            a.K(b, c, d, g)
        }
    };

    function pM(a, b) {
        this.A = a;
        this.B = b
    }
    pM.prototype.cb = ca(null);
    pM.prototype.Na = function() {
        return {
            ptrin: {
                qa: ["mouseover"],
                global: null
            },
            ptrout: {
                qa: ["mouseout"],
                global: null
            }
        }
    };
    pM.prototype.nb = C;
    pM.prototype.mb = function(a, b) {
        var c = b.event(),
            d = c.type;
        if ("mouseover" == d) {
            var d = c.relatedTarget || null,
                c = c.target,
                e = b.event();
            e.Nk = d;
            e.Kk = c;
            e.Wd = this.B.Wd();
            this.A(a, "ptrin", b, e)
        } else "mouseout" == d && (d = c.target, c = c.relatedTarget || null, e = b.event(), e.Nk = d, e.Kk = c, e.Wd = this.B.Wd(), this.A(a, "ptrout", b, e))
    };

    function qM(a, b, c) {
        this.A = a;
        this.B = b;
        this.C = c
    }
    qM.prototype.cb = function() {
        return {
            scroll: ["scroll"]
        }
    };
    qM.prototype.Na = function() {
        return {
            scroll: {
                qa: ["scroll"],
                global: null
            }
        }
    };
    qM.prototype.nb = function(a) {
        if ("scroll" === a.type) {
            var b = new wo(this.A, "scroll");
            this.C("scroll", b, a);
            b.done("main-actionflow-branch")
        }
    };
    qM.prototype.mb = function(a, b) {
        b.event();
        if ("scroll" == b.event().type) {
            var c = b.event();
            this.B(a, "scroll", b, c)
        }
    };

    function rM() {
        this.A = [];
        this.B = !1
    }
    rM.prototype.filter = function(a) {
        if (!(Jd || Kd || Fd)) return !1;
        a = new sM(F(), a.A);
        if (0 < this.A.length) {
            var b = this.A[this.A.length - 1],
                c = 0 > a.B != 0 > b.B;
            if (100 < a.A - b.A || c) this.A.length = 0
        }
        this.A.push(a);
        10 < this.A.length && this.A.shift();
        if (3 > this.A.length) this.B = !1;
        else {
            a = this.A;
            b = a.length;
            if (2 > b) a = NaN;
            else {
                for (var c = [0, 0, 0, 0, 0], d = a[0].A - 100, e, f, g = 0; g < b; g++)
                    if (e = a[g].A - d, f = Math.abs(a[g].B)) c[0] += e, c[1] += f, c[2] += e * e, c[3] += e * f, c[4] += f * f;
                a = c[1] / b - (b * c[3] - c[0] * c[1]) / (b * c[2] - c[0] * c[0]) * c[0] / b
            }
            this.B = this.B ? 0 < a : 15 < a
        }
        return this.B
    };

    function sM(a, b) {
        this.A = a;
        this.B = b
    };

    function tM(a, b, c, d, e) {
        Ik.call(this, a);
        this.type = "wheel";
        this.deltaMode = b;
        this.deltaX = c;
        this.deltaY = d;
        this.deltaZ = e;
        a = 1;
        switch (b) {
            case 2:
                a *= 450;
                break;
            case 1:
                a *= 15
        }
        this.B = this.deltaX * a;
        this.A = this.deltaY * a
    }
    H(tM, Ik);

    function uM(a, b) {
        il.call(this);
        a = this.A = a;
        a = za(a) && 1 == a.nodeType ? this.A : this.A.body;
        this.C = !!a && "rtl" == Un(a, "direction");
        this.B = Vk(this.A, vM(), this, b)
    }
    H(uM, il);

    function vM() {
        return Tc && ed(17) || Rc && ed(9) || Jd && 0 <= db(iE, 31) ? "wheel" : Tc ? "DOMMouseScroll" : "mousewheel"
    }
    uM.prototype.handleEvent = function(a) {
        var b = 0,
            c = 0,
            d = 0,
            e = 0;
        a = a.ab;
        "wheel" == a.type ? (b = a.deltaMode, c = a.deltaX, d = a.deltaY, e = a.deltaZ) : "mousewheel" == a.type ? y(a.wheelDeltaX) ? (c = -a.wheelDeltaX, d = -a.wheelDeltaY) : d = -a.wheelDelta : (b = 1, y(a.axis) && a.axis === a.HORIZONTAL_AXIS ? c = a.detail : d = a.detail);
        this.C && (c = -c);
        b = new tM(a, b, c, d, e);
        this.dispatchEvent(b)
    };
    uM.prototype.ea = function() {
        uM.V.ea.call(this);
        dl(this.B);
        this.B = null
    };

    function wM() {
        this.A = xM()
    }

    function xM() {
        if (Xc) {
            if (Jd || Qc) return 100;
            if (Fd) return 45;
            if (Rc) return 49.95
        } else if (Wc) {
            if (!(Jd || Qc || Kd) && Fd) return 20
        } else if (Yc) {
            if (Jd || Qc) return 53;
            if (Fd) return 45
        }
        return 50
    };

    function yM(a) {
        this.C = a;
        this.B = new uM(new il);
        this.B.addEventListener("wheel", E(ba("A"), this));
        this.A = null;
        this.F = new rM;
        this.D = new wM
    }
    yM.prototype.cb = ca(null);
    yM.prototype.Na = function() {
        var a = {},
            b = vM();
        a.scrollwheel = {
            qa: [b],
            global: null
        };
        return a
    };
    yM.prototype.nb = C;
    yM.prototype.mb = function(a, b) {
        var c = new Ik(b.event());
        this.B.handleEvent(c);
        var d = this.A,
            e = LL(c, b.node().getBoundingClientRect());
        c.x = e.x;
        c.y = e.y;
        c.ef = d.B;
        c.Cd = d.A;
        c.Lk = d.A / this.D.A;
        c.ctrlKey = d.ctrlKey;
        c.Ji = this.F.filter(d);
        this.C(a, "scrollwheel", b, c)
    };

    function zM(a, b) {
        this.G = b;
        b = null;
        YL() && (b = new WL);
        this.H = b;
        this.A = new OL(a, E(this.qk, this));
        this.D = (a = XL()) ? "MSPointerDown" : "pointerdown";
        this.B = a ? "MSPointerMove" : "pointermove";
        this.F = a ? "MSPointerUp" : "pointerup";
        this.C = a ? "MSPointerCancel" : "pointercancel"
    }
    q = zM.prototype;
    q.cb = ca(null);
    q.Na = function() {
        var a = ["touchstart", "touchmove", "touchend", "touchcancel"];
        YL() && (a = a.concat([this.D, this.B, this.F, this.C]));
        var a = {
                qa: a,
                global: null
            },
            b = {};
        b.dragstart = a;
        b.drag = a;
        b.dragend = a;
        return b
    };
    q.nb = C;
    q.mb = function(a, b) {
        b.event();
        var c = b.event();
        if (this.H) {
            var d = c.type;
            if (d == this.D || d == this.B || d == this.F || d == this.C) c = bM(this.H, c);
            if (!c) return
        }
        var d = c.touches,
            e = c.type;
        c.preventDefault();
        if ("touchstart" == e) NL(c.target), PL(this.A, a, d[0], b.node()), QL(this.A, c, d[0]);
        else if ("touchmove" == e) QL(this.A, c, d[0]);
        else if ("touchcancel" == e || "touchend" == e) 0 == d.length ? !TL(this.A, c) && (d = b.node()) && (c = new Ik(b.event()), d = LL(c, d.getBoundingClientRect()), c.x = d.x, c.y = d.y, this.G(a, "click", b, c)) : QL(this.A, c, d[0])
    };
    q.qk = function(a, b, c, d) {
        this.G(a, b, c, d, 0)
    };

    function AM(a, b, c, d) {
        this.I = a;
        this.B = null;
        KL() && (this.B = new JL);
        qo && Qo(qo, "beforedone", FL);
        c = E(this.Yf, this);
        var e = E(this.Yf, this, ""),
            f = [],
            g = new UL(b, c);
        f.push(g);
        f.push(new zM(b, c));
        g = new dM(c, g);
        f.push(g);
        f.push(new pM(c, g));
        f.push(new lL(b, e));
        f.push(new yM(c));
        f.push(new AL(c));
        f.push(new BL(c));
        f.push(new GL(b, c, e));
        f.push(new qM(b, c, e));
        f.push(new mL(c));
        this.A = f;
        this.G = new fj;
        this.H = new fj;
        this.F = {};
        this.C = [new wL(this.A, a, b), new CL(this.A, d)];
        this.D = {};
        a = 0;
        for (b = this.A.length; a < b; ++a)(d = this.A[a].Na()) &&
            gj(this.G, Pb(d)), (d = this.A[a].cb()) && gj(this.H, Pb(d))
    }
    q = AM.prototype;
    q.Xc = function(a) {
        var b = this.D[a];
        if (b) {
            var c = b.Wb,
                d = b.rb,
                b = b.qualifier;
            delete this.D[a];
            a = this.F;
            delete a[c][d][b];
            Qb(a[c][d]) && (delete a[c][d], Qb(a[c]) && delete a[c]);
            a = 0;
            for (b = this.C.length; a < b; ++a) this.C[a].H(c, d, !1)
        }
    };
    q.Qb = function(a, b, c, d) {
        return this.H.contains(a) ? BM(this, "", a, b, c, y(d) ? d : null) : null
    };
    q.wc = function(a, b, c, d, e, f) {
        return this.G.contains(c) ? BM(this, a ? a + "." + b : b, c, d, e, y(f) ? f : null) : null
    };

    function BM(a, b, c, d, e, f) {
        a: for (var g = 0, k = IC.length; g < k; ++g)
            if (c == IC[g]) break a;g = a.F;g[b] = g[b] || {};g[b][c] = g[b][c] || {};g[b][c][f] = d ? {
            Oc: e,
            scope: d
        } : e;d = 0;
        for (e = a.C.length; d < e; ++d) a.C[d].H(b, c, !0);d = ++CM;a.D[d] = {
            Wb: b,
            rb: c,
            qualifier: f
        };
        return d
    }
    q.Yf = function(a, b, c, d, e) {
        var f = this.F;
        f[a] && f[a][b] ? (a = f[a][b], e = a[y(e) ? e : null] || a.all_others || a[null]) : e = null;
        if (e) {
            if (this.B) var g = this.B.getTime();
            ya(e) ? e(c, d) : e.Oc.call(e.scope, c, d);
            this.B && (d = this.B.getTime() - g, .75 > d || (c = Aa(c), y(EL[c]) || y(DL[c]) ? y(EL[c]) && y(DL[c]) && (EL[c] += d, DL[c]++) : (EL[c] = d, DL[c] = 1)))
        }
    };
    q.tc = h("I");
    var CM = 0;

    function DM() {
        this.A = [];
        this.B = []
    }

    function EM(a, b) {
        a.B.push(b)
    }

    function FM(a) {
        0 == a.A.length && (a.A = a.B, a.A.reverse(), a.B = []);
        return a.A.pop()
    }
    q = DM.prototype;
    q.Bb = function() {
        return this.A.length + this.B.length
    };
    q.Oa = function() {
        return 0 == this.A.length && 0 == this.B.length
    };
    q.clear = function() {
        this.A = [];
        this.B = []
    };
    q.contains = function(a) {
        return sb(this.A, a) || sb(this.B, a)
    };
    q.remove = function(a) {
        var b;
        b = this.A;
        var c;
        b: if (c = b.length - 1, 0 > c && (c = Math.max(0, b.length + c)), wa(b)) c = wa(a) && 1 == a.length ? b.lastIndexOf(a, c) : -1;
            else {
                for (; 0 <= c; c--)
                    if (c in b && b[c] === a) break b;
                c = -1
            }
        0 <= c ? (ub(b, c), b = !0) : b = !1;
        return b || tb(this.B, a)
    };
    q.Ca = function() {
        for (var a = [], b = this.A.length - 1; 0 <= b; --b) a.push(this.A[b]);
        for (var c = this.B.length, b = 0; b < c; ++b) a.push(this.B[b]);
        return a
    };

    function GM() {
        this.A = {};
        this.B = this.C = void 0
    }

    function HM(a, b, c) {
        c = Math.floor(c);
        a.A[c] || (a.A[c] = new DM);
        EM(a.A[c], b);
        if (!y(a.C) || c < a.C) a.C = c;
        if (!y(a.B) || c > a.B) a.B = c
    }

    function IM(a) {
        a: {
            if (y(a.B))
                for (var b = a.B; b >= a.C; b--)
                    if (a.A[b] && !a.A[b].Oa()) {
                        a = a.A[b];
                        break a
                    }
            a = null
        }
        return a ? FM(a) : void 0
    }
    GM.prototype.remove = function(a) {
        if (y(this.B))
            for (var b = this.B; b >= this.C && (!this.A[b] || !this.A[b].remove(a)); b--);
    };

    function JM(a) {
        if (!y(a.B)) return -1;
        for (var b = a.B; b >= a.C; b--)
            if (a.A[b] && !a.A[b].Oa()) return b;
        return -1
    };

    function KM(a, b) {
        this.K = a;
        this.B = 0;
        this.A = [];
        this.I = null != b ? b : 24;
        this.D = 0;
        this.F = new GM;
        this.G = {};
        this.C = -1;
        this.H = void 0
    }

    function LM(a, b) {
        this.B = a;
        this.A = this.D = this.C = !1;
        this.Qa = b;
        this.F = 0
    }

    function Qr(a, b, c) {
        var d = b.state;
        if (!d || d.Qa != c) {
            if (d) a: if (b = d, b.C) {
                for (var d = a.F, e = Math.floor(c), f = d.B; f >= d.C; f--)
                    if (d.A[f] && d.A[f].remove(b)) {
                        HM(d, b, e);
                        break
                    }
                b.Qa = c
            } else {
                if (b.D) {
                    d = c > b.Qa;
                    e = 0 == a.D;
                    f = JM(a.F) <= c;
                    if (d || e || f) {
                        MM(a, b);
                        NM(a, b, c);
                        break a
                    }
                    a.remove(b.B) && OM(a, b, c)
                }
                b.A && (a.remove(b.B), OM(a, b, c))
            }
            else d = new LM(b, c), b.state = d, OM(a, d, c);
            if (0 != a.I && a.B == a.I)
                for (b = !1, d = 1; d < c; d++) {
                    if (a.A[d] && 0 < a.A[d].length)
                        for (var e = a.A[d], f = e.length - 1, g; g = e[f]; f--)
                            if (g.B.cancel()) {
                                b = !0;
                                g.B.state && MM(a, g);
                                OM(a,
                                    g, d);
                                break
                            }
                    if (b) break
                }
            PM(a)
        }
    }
    KM.prototype.start = function() {
        for (var a = 0; 4 > a && QM(this); ++a) {
            var b;
            for (b = IM(this.F); b && !b.C;) b = IM(this.F);
            b ? (b.C = !1, this.D += -1) : b = null;
            if (!b) break;
            RM(this, b)
        }
        for (a = 3; 1 <= a && !(this.A[a] && 0 < this.A[a].length); a--);
        for (b = 1; 3 >= b; b++)
            if (b < a) {
                var c = this.G[b];
                if (c && 0 < c.length)
                    for (; 0 < c.length;) {
                        var d = c.pop();
                        d.A = !1;
                        d.B.cancel();
                        OM(this, d, b)
                    }
            }
        if (QM(this)) return this.start;
        this.H = void 0;
        return rj
    };

    function QM(a) {
        var b = -1 != JM(a.F);
        a = 0 == a.I || a.B < a.I;
        return b && a
    }

    function PM(a) {
        var b = JM(a.F);
        if (-1 != b) {
            var c = 0;
            2 == b ? c = 1 : 3 == b && (c = 2);
            if (!y(a.H)) jq(a.K, a, kq(c, !0)), a.H = c;
            else if (a.H < c) {
                var b = a.K,
                    d = a.__maps_realtime_JobScheduler_next_step;
                if (d && d != rj) {
                    var d = a.__maps_realtime_JobScheduler_priority,
                        e;
                    e = 1 == d || 3 == d || 5 == d ? kq(c, !0) : kq(c, !1);
                    if (d != e) {
                        for (var f = b.A[d].length, g = 0; g < f; ++g)
                            if (b.A[d][g] == a) {
                                b.A[d][g] = null;
                                break
                            }
                        a.__maps_realtime_JobScheduler_priority = e;
                        b.A[e].push(a)
                    }
                }
                a.H = c
            }
        }
    }

    function RM(a, b) {
        NM(a, b, b.Qa);
        b.B.start(function() {
            SM(a, b)
        })
    }

    function OM(a, b, c) {
        b.Qa = c;
        b.C = !0;
        a.D += 1;
        HM(a.F, b, c)
    }

    function MM(a, b) {
        a.A[b.Qa] && tb(a.A[b.Qa], b);
        b.D = !1;
        a.B += -1;
        0 == a.B && -1 != a.C && (w.clearTimeout(a.C), a.C = -1)
    }

    function NM(a, b, c) {
        a.A[c] ? a.A[c].push(b) : a.A[c] = [b];
        b.F = F();
        b.D = !0;
        a.B += 1;
        b.Qa = c; - 1 == a.C && TM(a)
    }

    function TM(a) {
        a.C = w.setTimeout(function() {
            if (0 < a.B && -1 != a.C) {
                for (var b = F(), c = [], d = 1; 3 >= d; d++) {
                    var e = a.A[d];
                    if (e)
                        for (var f = 0; f < e.length; ++f) {
                            var g = e[f];
                            1E4 <= b - g.F && c.push(g)
                        }
                }
                for (b = 0; b < c.length; ++b) d = a, e = c[b], MM(d, e), d.G[e.Qa] ? d.G[e.Qa].push(e) : d.G[e.Qa] = [e], e.A = !0, PM(d);
                0 < a.B ? TM(a) : a.C = -1
            }
        }, 1E4)
    }

    function SM(a, b) {
        b && (b.D ? MM(a, b) : b.A && (tb(a.G[b.Qa], b), b.A = !1), b.B.state = null);
        PM(a)
    }
    KM.prototype.remove = function(a) {
        var b = a.state,
            c = !1;
        if (b && (b.D || b.A)) {
            if (a.cancel() || b.A) SM(this, b), c = !0
        } else b && b.C && (b.C = !1, this.D += -1, c = !0);
        c && (a.state = null);
        return c
    };

    function UM(a, b, c, d) {
        this.H = !!a;
        this.M = c || C;
        this.F = this.G = !1;
        this.D = null;
        this.N = b ? b : w.requestAnimationFrame || w.webkitRequestAnimationFrame || w.mozRequestAnimationFrame || w.oRequestAnimationFrame || w.msRequestAnimationFrame || function(a) {
            w.setTimeout(a, 16)
        };
        var e = this;
        this.J = function() {
            e.A = !1;
            VM(e, !1)
        };
        this.K = function() {
            e.C = !1;
            e.G = !1;
            VM(e, e.H && e.A)
        };
        this.C = this.A = !1;
        this.B = !0;
        this.I = 0;
        gK(E(this.L, this), void 0, d)
    }

    function VM(a, b) {
        a.F = !0;
        try {
            if (b) {
                var c = a.D;
                try {
                    c.H = !0;
                    var d = F() + 2;
                    for (b = 5; 0 <= b && WM(c, b, d); b--);
                } finally {
                    c.H = !1, (0 < c.A[5].length || 0 < c.A[4].length || 0 < c.A[3].length || 0 < c.A[2].length || 0 < c.A[1].length || 0 < c.A[0].length) && yE(c.B), c.aa++
                }
            } else a.D.wa()
        } catch (e) {
            throw a.M(e), e;
        }
        a.G && (!a.H && a.A && a.B || XM(a));
        a.F = !1
    }

    function ZD(a) {
        a.B ? a.A || !a.H && a.C || (a.N.call(w, a.J), a.A = !0) : XM(a)
    }

    function yE(a) {
        Kd ? ZD(a) : a.F ? a.G = !0 : a.B && a.A || XM(a)
    }

    function XM(a) {
        a.C || !a.B && F() > a.I || (Eq(a.K), a.C = !0)
    }
    UM.prototype.L = function(a) {
        (this.B = a) && !this.A ? XM(this) : this.I = F() + 1E4
    };

    function YM(a, b, c) {
        this.B = a || new UM;
        this.B.D = this;
        this.da = F();
        this.P = ZM;
        this.$ = this.C = this.ha = this.aa = this.ga = this.X = this.K = this.D = this.F = 0;
        this.N = this.H = !1;
        this.J = [];
        this.I = [];
        this.G = [];
        this.A = [];
        this.A[0] = [];
        this.A[1] = [];
        this.A[2] = [];
        this.A[3] = [];
        this.A[4] = [];
        this.A[5] = [];
        this.U = [];
        this.na = !!b;
        this.ja = !c;
        this.M = !1;
        this.O = this.Y = this.L = 0;
        sj.push(this)
    }
    var ZM = 1E3 / 60,
        $M = 1E4 / 60;

    function kq(a, b) {
        a *= 2;
        b && (a += 1);
        return a
    }

    function jq(a, b, c) {
        var d = b.__maps_realtime_JobScheduler_next_step;
        d && d != rj || (b.__maps_realtime_JobScheduler_next_step = b.start, b.__maps_realtime_JobScheduler_priority = c, a.A[c].push(b), a.H || yE(a.B))
    }
    YM.prototype.wa = function() {
        var a = F();
        this.H = !0;
        var b = 0,
            c = this.U;
        if (0 < c.length) {
            for (b = 0; b < c.length; b++) jq(this, c[b].fk, kq(c[b].Qa, !1));
            this.U = []
        }
        try {
            var d = F(),
                e = this.J;
            this.J = [];
            for (var f = e.length, c = 0; c < f; c++) {
                var g = e[c];
                aN();
                var k = g;
                k.B = !1;
                var l = k.C;
                k.C = [];
                for (var m = 0; m < l.length; ++m) {
                    var n = l[m],
                        p = k.A[n.type][n.qualifier];
                    if (p && 0 < p.length)
                        for (var r = m + 1 < l.length ? l[m + 1] : null, u = r && r.type == n.type && r.qualifier == n.qualifier && (null == r.event || null == n.event || r.event.type == n.event.type), t = 0; t < p.length; ++t) {
                            var v =
                                p[t];
                            u && v.A || v.gb(n.oa, n.event)
                        }
                    n.oa.done("scene-async-event-handler")
                }
            }
            this.C += F() - d;
            var x;
            if (this.na) bN(this, a), cN(this, a), x = Infinity;
            else if (this.ja)
                if (a - this.X < this.P - (6 + this.F)) x = a + this.P - 3;
                else {
                    bN(this, a);
                    cN(this, a);
                    var D = F(),
                        d = D - a;
                    this.F *= .97;
                    this.F += .03 * d;
                    var z = Math.ceil(1 / ZM * (this.F + 3 + 6)) * ZM,
                        z = z < ZM ? ZM : z;
                    this.P = z = z > $M ? $M : z;
                    x = dN(this, a, D)
                }
            else {
                bN(this, a);
                cN(this, a);
                var A = F();
                x = dN(this, a, A)
            }
            this.N = !1;
            for (b = 5; 0 <= b && WM(this, b, x); b--);
        } finally {
            this.H = !1, b = 0 < this.G.length || 0 < this.I.length || 0 < this.J.length,
                x = 0 < this.A[5].length || 0 < this.A[4].length || 0 < this.A[3].length || 0 < this.A[2].length || 0 < this.A[1].length || 0 < this.A[0].length, b ? ZD(this.B) : x && yE(this.B), this.M && (this.O += F() - a), this.M = x || b, this.aa++
        }
    };

    function bN(a, b) {
        var c = F(),
            d = a.I;
        a.I = [];
        a.da = b;
        for (var e = d.length, f = 0; f < e; f++) {
            var g = d[f];
            aN();
            g.C(b)
        }
        a.C += F() - c
    }

    function dN(a, b, c) {
        a = b + a.P - 3;
        c -= a;
        0 < c && (a += Math.ceil(c / ZM) * ZM);
        return a
    }

    function WM(a, b, c) {
        var d = F();
        if (a.N && F() >= c) return !1;
        var e = a.A[b];
        if (0 == e.length) return !0;
        for (var f = [], g = !1, k = 0; k < e.length && !g; k++) {
            var l = e[k];
            if (l)
                for (;;) {
                    var m = l.__maps_realtime_JobScheduler_next_step;
                    if (!m || m == rj) break;
                    m = F();
                    if (a.N && m >= c) {
                        g = !0;
                        f.push(k);
                        break
                    }
                    aN();
                    m = rj;
                    try {
                        m = l.__maps_realtime_JobScheduler_next_step()
                    } finally {
                        l.__maps_realtime_JobScheduler_next_step = m, a.N = !0
                    }
                    if (m == rj) break
                }
        }
        a.$ += F() - d;
        d = [];
        for (m = 0; m < f.length; m++)
            if (l = e[f[m]]) {
                var n = l.__maps_realtime_JobScheduler_next_step;
                n && n !=
                    rj && d.push(l)
            }
        if (g) return a.A[b] = d.concat(e.slice(k - 1)), !1;
        a.A[b] = d;
        return F() < c
    }

    function cN(a, b) {
        var c = F(),
            d = b - a.X;
        if (a.M) {
            var e = 1E3 / d;
            d > tj && a.ha++;
            a.D *= .7;
            a.D += .3 * e;
            a.K *= .7;
            a.K += .3 * e * e;
            a.ga++;
            a.L += d;
            a.Y += a.O;
            a.O = 0
        }
        a.X = b;
        b = a.G;
        a.G = [];
        d = b.length;
        for (e = 0; e < d; e++) {
            var f = b[e];
            aN();
            f.H()
        }
        a.C += F() - c
    }

    function xE(a) {
        if (0 < a.G.length || 0 < a.I.length || 0 < a.J.length) return !0;
        if (!y(1)) return !1;
        for (var b = kq(1, !1); 5 >= b; b++)
            if (a.A[b].length) return !0;
        return !1
    }

    function aN() {
        w.performance && w.performance.now ? w.performance.now() : F()
    }
    YM.prototype.xa = C;

    function eN(a, b, c, d, e, f, g, k, l, m, n, p, r, u, t, v) {
        this.U = a;
        this.C = b;
        this.A = c;
        this.J = d || null;
        this.da = e || null;
        this.F = f || null;
        this.N = g || null;
        this.K = k || null;
        this.O = l || null;
        this.aa = m || null;
        this.X = n || null;
        this.P = p || null;
        this.I = r || null;
        this.B = u || null;
        this.M = t || null;
        this.L = v || null;
        this.H = new YM;
        this.G = fN();
        this.$ = new LC(this.G);
        this.Y = new KM(this.H);
        this.D = !(!$K() && (Yb("iPod") || Yb("iPhone") || Yb("Android") || Yb("IEMobile"))) && !$K()
    }

    function fN() {
        var a = new bL,
            b = Po(),
            a = new AM(a, b);
        a.Qb("beforeunload", null, function(a) {
            To(No, "beforeunload", a)
        });
        return a
    }
    eN.prototype.getContext = h("A");

    function gN(a, b) {
        Bj || (Bj = new ZK);
        uj("CPNR", vq);
        uj("CUTS", Wr);
        uj("CUCS", jt);
        uj("CTS", pt);
        uj("FPSC", ov);
        uj("FPTS", iw);
        uj("GCS", rx);
        uj("HPNR", Ew);
        uj("SCPI", Fz);
        uj("PNI", Hz);
        uj("SCPR", OD);
        uj("SCW", wK);
        uj("WPNR", YK);
        return new hN(a.A, b)
    };

    function iN(a) {
        a = Math.max(a, -90);
        return a = Math.min(a, 90)
    };

    function jN() {}

    function kN() {}
    jN = function(a, b) {
        if (!a) throw Error(b || "Precondition check failed on argument");
    };
    kN = function(a, b) {
        jN(typeof a == b, "Argument expected to be of type " + b)
    };

    function lN(a) {
        this.message = a;
        this.name = "InvalidValueError";
        this.stack = Error().stack
    }
    H(lN, Error);

    function mN(a, b) {
        var c = "";
        if (null != b) {
            if (!(b instanceof lN)) return b;
            c = ": " + b.message
        }
        return new lN(a + c)
    };

    function nN(a, b, c) {
        this.heading = a;
        this.pitch = iN(b);
        this.zoom = Math.max(0, c)
    };

    function oN(a) {
        this.data = a || []
    }
    H(oN, J);

    function pN(a) {
        this.data = a || []
    }
    H(pN, J);

    function qN(a) {
        this.data = a || []
    }
    H(qN, J);

    function rN(a) {
        a.handled = !0;
        y(a.bubbles) || (a.returnValue = "handled")
    };
    var sN = "undefined" != typeof navigator && -1 != navigator.userAgent.toLowerCase().indexOf("msie"),
        tN = {};

    function uN(a, b) {
        a.__e3_ || (a.__e3_ = {});
        a = a.__e3_;
        a[b] || (a[b] = {});
        return a[b]
    }

    function vN(a, b, c, d) {
        jN(a);
        kN(c, "function");
        this.Xa = a;
        this.A = b;
        this.B = c;
        this.C = null;
        this.D = d;
        this.id = ++wN;
        uN(a, b)[this.id] = this;
        sN && "tagName" in a && (tN[this.id] = this)
    }
    var wN = 0;

    function xN(a) {
        return a.C = function(b) {
            b || (b = window.event);
            if (b && !b.target) try {
                b.target = b.srcElement
            } catch (d) {}
            var c;
            c = a.B.apply(a.Xa, [b]);
            return b && "click" == b.type && (b = b.srcElement) && "A" == b.tagName && "javascript:void(0)" == b.href ? !1 : c
        }
    }
    vN.prototype.remove = function() {
        if (this.Xa) {
            switch (this.D) {
                case 1:
                    this.Xa.removeEventListener(this.A, this.B, !1);
                    break;
                case 4:
                    this.Xa.removeEventListener(this.A, this.B, !0);
                    break;
                case 2:
                    this.Xa.detachEvent("on" + this.A, this.C);
                    break;
                case 3:
                    this.Xa["on" + this.A] = null
            }
            delete uN(this.Xa, this.A)[this.id];
            this.C = this.B = this.Xa = null;
            delete tN[this.id]
        }
    };

    function yN(a, b, c) {
        this.Sa = a;
        this.yb = b;
        this.A = c
    };

    function zN(a, b, c) {
        this.B = a;
        this.F = b;
        this.D = c;
        this.G = this.B.createTexture();
        this.M = this.J = 10497;
        this.K = 9986;
        this.I = 9729;
        this.C = 0;
        this.A = 3553;
        this.O = this.H = 0;
        this.N = !1;
        this.L = 34069
    }
    zN.prototype.bind = function() {
        3553 == this.A ? this.D.jc(this.C, this) : this.D.kc(this.C, this)
    };

    function AN(a, b) {
        a.J != b && (a.bind(), a.B.texParameteri(a.A, 10242, b), a.J = b)
    }

    function BN(a, b) {
        a.M != b && (a.bind(), a.B.texParameteri(a.A, 10243, b), a.M = b)
    }

    function CN(a, b) {
        a.K != b && (a.bind(), a.B.texParameteri(a.A, 10241, b), a.K = b)
    }

    function DN(a, b) {
        a.I != b && (a.bind(), a.B.texParameteri(a.A, 10240, b), a.I = b)
    }
    zN.prototype.deleteTexture = function() {
        for (var a = this.F.jb(), b = 0; b <= this.D.ig(); ++b) this.C != b && (this.C = b), 3553 == this.A ? this.D.B[this.C] == this && this.D.jc(this.C, null) : this.D.C[this.C] == this && this.D.kc(this.C, null);
        this.N = !0;
        this.B.deleteTexture(this.G);
        this.F.Sb(a)
    };
    zN.prototype.W = h("H");
    zN.prototype.generateMipmap = function() {
        if (34067 == this.A)
            for (var a = 0; 6 > a; ++a);
        this.bind();
        this.B.generateMipmap(this.A)
    };

    function FK(a, b, c, d, e) {
        EN(a, b.width, b.height, c, d, e);
        var f = FN(a);
        a.bind();
        GN(a, b.width, c, d);
        a.B.texImage2D(f, e, c, c, d, b);
        a.F.zb(3317)
    }

    function EN(a, b, c, d, e, f) {
        0 != f || b == a.H && c == a.O && d == a.P && e == a.U || (a.H = b, a.O = c)
    }

    function FN(a) {
        return 34067 == a.A ? a.L : a.A
    }
    var HN = {
            6408: 4,
            6407: 3,
            6410: 2,
            6409: 1,
            6406: 1
        },
        IN = {
            5121: 1,
            5126: 4,
            32819: 2,
            33635: 2,
            32820: 2
        };

    function GN(a, b, c, d) {
        b *= (5121 == d || 5126 == d ? HN[c] : 1) * IN[d];
        0 != b % 4 && (c = 1, 0 == b % 2 && (c = 2), a.F.hb(3317, c))
    };

    function JN() {
        this.H = this.G = this.F = this.D = void 0;
        this.B = [];
        this.C = []
    }
    q = JN.prototype;
    q.clear = function() {
        this.vf();
        this.If();
        this.Jf();
        this.Of();
        for (var a = 31; 0 <= a; --a) this.Rf(a), this.Sf(a)
    };
    q.apply = function(a) {
        void 0 !== a.D && a.D !== this.D && this.dd(a.D);
        void 0 !== a.F && a.F !== this.F && this.rd(a.F);
        void 0 !== a.G && a.G !== this.G && this.sd(a.G);
        void 0 !== a.H && a.H !== this.H && this.xd(a.H);
        for (var b = 31; 0 <= b; --b) void 0 !== a.B[b] && a.B[b] !== this.B[b] && this.jc(b, a.B[b]), void 0 !== a.C[b] && a.C[b] !== this.C[b] && this.kc(b, a.C[b])
    };
    q.ig = ca(32);
    q.dd = ba("D");
    q.vf = function() {
        this.D = void 0
    };
    q.rd = ba("F");
    q.If = function() {
        this.F = void 0
    };
    q.sd = ba("G");
    q.Jf = function() {
        this.G = void 0
    };
    q.xd = ba("H");
    q.Of = function() {
        this.H = void 0
    };
    q.jc = function(a, b) {
        this.B[a] = b
    };
    q.Rf = function(a) {
        delete this.B[a]
    };
    q.kc = function(a, b) {
        this.C[a] = b
    };
    q.Sf = function(a) {
        delete this.C[a]
    };

    function KN(a, b) {
        JN.call(this);
        this.A = a;
        this.K = Math.min(32, a.getParameter(35661));
        this.J = b;
        this.I = Kd || Uc && !ed("536.3");
        a = this.A;
        this.A = null;
        this.clear();
        this.A = a
    }
    H(KN, JN);
    q = KN.prototype;
    q.dd = function(a) {
        if (this.I || this.D !== a) KN.V.dd.call(this, a), this.A && this.A.bindBuffer(34962, a)
    };
    q.vf = function() {
        this.dd(null)
    };
    q.rd = function(a) {
        if (this.I || this.F !== a) KN.V.rd.call(this, a), this.A && this.A.bindBuffer(34963, a)
    };
    q.If = function() {
        this.rd(null)
    };
    q.sd = function(a) {
        if (this.I || this.G !== a) KN.V.sd.call(this, a), this.A && this.A.bindFramebuffer(36160, a)
    };
    q.Jf = function() {
        this.sd(null)
    };
    q.xd = function(a) {
        if (this.I || this.H !== a) KN.V.xd.call(this, a), this.A && this.A.bindRenderbuffer(36161, a)
    };
    q.Of = function() {
        this.xd(null)
    };
    q.jc = function(a, b) {
        a < this.K && this.J.Sb(33984 + a);
        if (this.I || this.B[a] !== b) KN.V.jc.call(this, a, b), this.A && (b ? this.A.bindTexture(3553, b.G) : this.A.bindTexture(3553, null))
    };
    q.Rf = function(a) {
        this.jc(a, null)
    };
    q.kc = function(a, b) {
        a < this.K && this.J.Sb(33984 + a);
        if (this.I || this.C[a] !== b) KN.V.kc.call(this, a, b), this.A && (b ? this.A.bindTexture(34067, b.G) : this.A.bindTexture(34067, null))
    };
    q.Sf = function(a) {
        this.kc(a, null)
    };
    q.ig = function() {
        return this.K - 1
    };

    function LN() {
        this.G = new ArrayBuffer(204);
        this.D = new Uint8Array(this.G);
        this.C = new Uint16Array(this.G);
        this.H = new Uint32Array(this.G);
        this.F = new Int32Array(this.G);
        this.B = new Float32Array(this.G);
        this.clear()
    }
    LN.prototype.clear = function() {
        this.pb(3042);
        this.pb(2884);
        this.pb(2929);
        this.pb(3024);
        this.pb(32823);
        this.pb(32926);
        this.pb(32928);
        this.pb(3089);
        this.pb(2960);
        this.xf();
        this.yf();
        this.zf();
        this.Ff();
        this.Pf();
        this.Af();
        this.Bf();
        this.Cf();
        this.Df();
        this.Gf();
        this.Hf();
        this.Qf();
        this.Kg();
        this.Ef();
        this.Kf();
        this.Mf();
        this.Nf();
        for (var a = 0; 32 > a; ++a) this.Tf(a);
        this.sf();
        this.zb(3317);
        this.zb(3333);
        this.zb(37440);
        this.zb(37441);
        this.zb(37443);
        this.Lf(33170)
    };
    LN.prototype.apply = function(a) {
        MN(a, 3042) && NN(a, 3042) != NN(this, 3042) && this.La(3042, NN(a, 3042));
        MN(a, 2884) && NN(a, 2884) != NN(this, 2884) && this.La(2884, NN(a, 2884));
        MN(a, 2929) && NN(a, 2929) != NN(this, 2929) && this.La(2929, NN(a, 2929));
        MN(a, 3024) && NN(a, 3024) != NN(this, 3024) && this.La(3024, NN(a, 3024));
        MN(a, 32823) && NN(a, 32823) != NN(this, 32823) && this.La(32823, NN(a, 32823));
        MN(a, 32926) && NN(a, 32926) != NN(this, 32926) && this.La(32926, NN(a, 32926));
        MN(a, 32928) && NN(a, 32928) != NN(this, 32928) && this.La(32928, NN(a, 32928));
        MN(a, 3089) &&
            NN(a, 3089) != NN(this, 3089) && this.La(3089, NN(a, 3089));
        MN(a, 2960) && NN(a, 2960) != NN(this, 2960) && this.La(2960, NN(a, 2960));
        if (0 <= a.B[3]) {
            var b = a.B[3],
                c = a.B[4],
                d = a.B[5],
                e = a.B[6];
            this.B[3] == b && this.B[4] == c && this.B[5] == d && this.B[6] == e || this.ed(b, c, d, e)
        }
        65535 == a.C[14] || ON(this, !1) == ON(a, !1) && ON(this, !0) == ON(a, !0) || (b = ON(a, !1), c = ON(a, !0), c == b && (c = void 0), this.Bc(b, c));
        65535 != a.C[16] && (b = a.C[16], c = a.C[17], d = a.C[18], e = a.C[19], this.C[16] != b || this.C[17] != c || this.C[18] != d || this.C[19] != e) && (d == b && e == c && (e = d = void 0),
            this.Cc(b, c, d, e));
        65535 != a.C[20] && PN(a) != PN(this) && this.md(PN(a));
        0 < a.D[48] && (b = a.B[11], c = 2 == a.D[48], this.B[11] == b && this.D[48] == (c ? 2 : 1) || this.yd(b, c));
        0 <= a.B[13] && (b = a.B[13], c = a.B[14], d = a.B[15], e = a.B[16], this.B[13] == b && this.B[14] == c && this.B[15] == d && this.B[16] == e || this.fd(b, c, d, e));
        0 <= a.B[17] && QN(a) != QN(this) && this.hd(QN(a));
        1 == a.D[76] && RN(a) != RN(this) && this.jd(RN(a));
        0 < a.D[80] && (b = 2 == a.D[80], c = 2 == a.D[81], d = 2 == a.D[82], e = 2 == a.D[83], SN(this, b, c, d, e) || this.kd(b, c, d, e));
        0 < a.D[84] && TN(a) != TN(this) && this.od(TN(a));
        0 <= a.B[22] && (b = a.B[22], c = a.B[23], this.B[22] == b && this.B[23] == c || this.qd(b, c));
        0 <= a.F[26] && (b = a.F[24], c = a.F[25], d = a.F[26], e = a.F[27], this.F[24] == b && this.F[25] == c && this.F[26] == d && this.F[27] == e || this.zd(b, c, d, e));
        0 <= a.F[30] && (b = a.F[28], c = a.F[29], d = a.F[30], e = a.F[31], this.F[28] == b && this.F[29] == c && this.F[30] == d && this.F[31] == e || this.Ad(b, c, d, e));
        65535 != a.C[64] && UN(a) != UN(this) && this.ld(UN(a));
        65535 != a.C[66] && VN(a) != VN(this) && this.td(VN(a));
        0 < a.B[34] && WN(a) != WN(this) && this.vd(WN(a));
        0 < a.D[148] && (b = a.B[35],
            c = a.B[36], 0 < this.D[148] && this.B[35] == b && this.B[36] == c || this.wd(b, c));
        for (b = 0; 32 > b; ++b) 0 < a.D[152 + b] && XN(a, b) != XN(this, b) && this.Dc(b, XN(a, b));
        65535 != a.C[92] && a.jb() != this.jb() && this.Sb(a.jb());
        65535 != a.C[94 + YN[3317]] && ZN(a, 3317) != ZN(this, 3317) && this.hb(3317, ZN(a, 3317));
        65535 != a.C[94 + YN[3333]] && ZN(a, 3333) != ZN(this, 3333) && this.hb(3333, ZN(a, 3333));
        65535 != a.C[94 + YN[37440]] && ZN(a, 37440) != ZN(this, 37440) && this.hb(37440, ZN(a, 37440));
        65535 != a.C[94 + YN[37441]] && ZN(a, 37441) != ZN(this, 37441) && this.hb(37441, ZN(a,
            37441));
        65535 != a.C[94 + YN[37443]] && ZN(a, 37443) != ZN(this, 37443) && this.hb(37443, ZN(a, 37443));
        65535 != a.C[100] && $N(a) != $N(this) && this.ud(33170, $N(a))
    };
    var aO = [];
    aO[3042] = 0;
    aO[2884] = 1;
    aO[2929] = 2;
    aO[3024] = 3;
    aO[32823] = 4;
    aO[32926] = 5;
    aO[32928] = 6;
    aO[3089] = 7;
    aO[2960] = 8;
    q = LN.prototype;
    q.La = function(a, b) {
        this.D[0 + aO[a]] = b ? 2 : 1
    };

    function NN(a, b) {
        a = a.D[0 + aO[b]];
        if (0 != a) return 2 == a
    }

    function MN(a, b) {
        return 0 < a.D[0 + aO[b]]
    }
    q.pb = function(a) {
        this.D[0 + aO[a]] = 0
    };
    q.ed = function(a, b, c, d) {
        this.B[3] = a;
        this.B[4] = b;
        this.B[5] = c;
        this.B[6] = d
    };
    q.xf = function() {
        this.B[3] = -1;
        this.B[4] = -1;
        this.B[5] = -1;
        this.B[6] = -1
    };
    q.Bc = function(a, b) {
        this.C[14] = a;
        this.C[15] = b || a
    };

    function ON(a, b) {
        a = b ? a.C[15] : a.C[14];
        if (65535 != a) return a
    }
    q.yf = function() {
        this.C[14] = 65535;
        this.C[15] = 65535
    };
    q.Cc = function(a, b, c, d) {
        this.C[16] = a;
        this.C[17] = b;
        this.C[18] = void 0 === c ? a : c;
        this.C[19] = void 0 === d ? b : d
    };
    q.zf = function() {
        this.C[16] = 65535;
        this.C[17] = 65535;
        this.C[18] = 65535;
        this.C[19] = 65535
    };
    q.md = function(a) {
        this.C[20] = a
    };

    function PN(a) {
        a = a.C[20];
        if (65535 != a) return a
    }
    q.Ff = function() {
        this.C[20] = 65535
    };
    q.yd = function(a, b) {
        this.B[11] = a;
        this.D[48] = b ? 2 : 1
    };
    q.Pf = function() {
        this.D[48] = 0
    };
    q.fd = function(a, b, c, d) {
        this.B[13] = a;
        this.B[14] = b;
        this.B[15] = c;
        this.B[16] = d
    };
    q.Af = function() {
        this.B[13] = -1;
        this.B[14] = -1;
        this.B[15] = -1;
        this.B[16] = -1
    };
    q.hd = function(a) {
        this.B[17] = a
    };

    function QN(a) {
        a = a.B[17];
        if (!(0 > a)) return a
    }
    q.Bf = function() {
        this.B[17] = -1
    };
    q.jd = function(a) {
        this.H[18] = a;
        this.D[76] = 1
    };

    function RN(a) {
        if (1 == a.D[76]) return a.H[18]
    }
    q.Cf = function() {
        this.D[76] = 0
    };
    q.kd = function(a, b, c, d) {
        this.D[80] = a ? 2 : 1;
        this.D[81] = b ? 2 : 1;
        this.D[82] = c ? 2 : 1;
        this.D[83] = d ? 2 : 1
    };
    q.Df = function() {
        this.D[80] = 0;
        this.D[81] = 0;
        this.D[82] = 0;
        this.D[83] = 0
    };

    function SN(a, b, c, d, e) {
        return a.D[80] == (b ? 2 : 1) && a.D[81] == (c ? 2 : 1) && a.D[82] == (d ? 2 : 1) && a.D[83] == (e ? 2 : 1)
    }
    q.od = function(a) {
        this.D[84] = a ? 2 : 1
    };

    function TN(a) {
        a = a.D[84];
        if (0 != a) return 2 == a
    }
    q.Gf = function() {
        this.D[84] = 0
    };
    q.qd = function(a, b) {
        this.B[22] = a;
        this.B[23] = b
    };
    q.Hf = function() {
        this.B[22] = -1;
        this.B[23] = -1
    };
    q.zd = function(a, b, c, d) {
        this.F[24] = a;
        this.F[25] = b;
        this.F[26] = c;
        this.F[27] = d
    };
    q.Qf = function() {
        this.F[26] = -1;
        this.F[27] = -1
    };
    q.Ad = function(a, b, c, d) {
        this.F[28] = a;
        this.F[29] = b;
        this.F[30] = c;
        this.F[31] = d
    };
    q.Kg = function() {
        this.F[30] = -1;
        this.F[31] = -1
    };
    q.ld = function(a) {
        this.C[64] = a
    };

    function UN(a) {
        a = a.C[64];
        if (65535 != a) return a
    }
    q.Ef = function() {
        this.C[64] = 65535
    };
    q.td = function(a) {
        this.C[66] = a
    };

    function VN(a) {
        a = a.C[66];
        if (65535 != a) return a
    }
    q.Kf = function() {
        this.C[66] = 65535
    };
    q.vd = function(a) {
        this.B[34] = a
    };

    function WN(a) {
        a = a.B[34];
        if (!(0 > a)) return a
    }
    q.Mf = function() {
        this.B[34] = -1
    };
    q.wd = function(a, b) {
        this.B[35] = a;
        this.B[36] = b;
        this.D[148] = 1
    };
    q.Nf = function() {
        this.D[148] = 0
    };
    q.Dc = function(a, b) {
        this.D[152 + a] = b ? 2 : 1
    };

    function XN(a, b) {
        a = a.D[152 + b];
        if (0 != a) return 2 == a
    }
    q.Tf = function(a) {
        this.D[152 + a] = 0
    };
    q.Sb = function(a) {
        this.C[92] = a
    };
    q.jb = function() {
        var a = this.C[92];
        if (65535 != a) return a
    };
    q.sf = function() {
        this.C[92] = 65535
    };
    var YN = [];
    YN[3317] = 0;
    YN[3333] = 1;
    YN[37440] = 2;
    YN[37441] = 3;
    YN[37443] = 4;
    LN.prototype.hb = function(a, b) {
        this.C[94 + YN[a]] = b
    };

    function ZN(a, b) {
        a = a.C[94 + YN[b]];
        if (65535 != a) return a
    }
    LN.prototype.zb = function(a) {
        this.C[94 + YN[a]] = 65535
    };
    LN.prototype.ud = function(a, b) {
        this.C[100] = b
    };

    function $N(a) {
        a = a.C[100];
        if (65535 != a) return a
    }
    LN.prototype.Lf = function() {
        this.C[100] = 65535
    };

    function bO(a) {
        this.A = a;
        a.scissor(0, 0, 0, 0);
        a.viewport(0, 0, 0, 0);
        a.enableVertexAttribArray(0);
        a = this.A;
        this.A = null;
        LN.call(this);
        this.A = a
    }
    H(bO, LN);
    q = bO.prototype;
    q.La = function(a, b) {
        if (NN(this, a) != b) {
            bO.V.La.call(this, a, b);
            var c = this.A;
            c && (b ? c.enable(a) : c.disable(a))
        }
    };
    q.pb = function(a) {
        3024 == a ? this.La(a, !0) : this.La(a, !1)
    };
    q.ed = function(a, b, c, d) {
        if (this.B[3] != a || this.B[4] != b || this.B[5] != c || this.B[6] != d) {
            bO.V.ed.call(this, a, b, c, d);
            var e = this.A;
            e && e.blendColor(a, b, c, d)
        }
    };
    q.xf = function() {
        this.ed(0, 0, 0, 0)
    };
    q.Bc = function(a, b) {
        var c = void 0 === b ? a : b;
        if (ON(this, !1) != a || ON(this, !0) != c) bO.V.Bc.call(this, a, b), (b = this.A) && (c == a ? b.blendEquation(a) : b.blendEquationSeparate(a, c))
    };
    q.yf = function() {
        this.Bc(32774)
    };
    q.Cc = function(a, b, c, d) {
        var e = void 0 === c ? a : c,
            f = void 0 === d ? b : d;
        if (this.C[16] != a || this.C[17] != b || this.C[18] != e || this.C[19] != f) bO.V.Cc.call(this, a, b, c, d), (c = this.A) && (e == a && f == b ? c.blendFunc(a, b) : c.blendFuncSeparate(a, b, e, f))
    };
    q.zf = function() {
        this.Cc(1, 0)
    };
    q.md = function(a) {
        if (PN(this) != a) {
            bO.V.md.call(this, a);
            var b = this.A;
            b && b.depthFunc(a)
        }
    };
    q.Ff = function() {
        this.md(513)
    };
    q.yd = function(a, b) {
        if (this.B[11] != a || this.D[48] != (b ? 2 : 1)) {
            bO.V.yd.call(this, a, b);
            var c = this.A;
            c && c.sampleCoverage(a, b)
        }
    };
    q.Pf = function() {
        this.yd(1, !1)
    };
    q.fd = function(a, b, c, d) {
        if (this.B[13] != a || this.B[14] != b || this.B[15] != c || this.B[16] != d) {
            bO.V.fd.call(this, a, b, c, d);
            var e = this.A;
            e && e.clearColor(a, b, c, d)
        }
    };
    q.Af = function() {
        this.fd(0, 0, 0, 0)
    };
    q.hd = function(a) {
        if (QN(this) != a) {
            bO.V.hd.call(this, a);
            var b = this.A;
            b && b.clearDepth(a)
        }
    };
    q.Bf = function() {
        this.hd(1)
    };
    q.jd = function(a) {
        if (RN(this) != a) {
            bO.V.jd.call(this, a);
            var b = this.A;
            b && b.clearStencil(a)
        }
    };
    q.Cf = function() {
        this.jd(0)
    };
    q.kd = function(a, b, c, d) {
        if (!SN(this, a, b, c, d)) {
            bO.V.kd.call(this, a, b, c, d);
            var e = this.A;
            e && e.colorMask(a, b, c, d)
        }
    };
    q.Df = function() {
        this.kd(!0, !0, !0, !0)
    };
    q.od = function(a) {
        if (TN(this) != a) {
            bO.V.od.call(this, a);
            var b = this.A;
            b && b.depthMask(a)
        }
    };
    q.Gf = function() {
        this.od(!0)
    };
    q.qd = function(a, b) {
        if (this.B[22] != a || this.B[23] != b) {
            bO.V.qd.call(this, a, b);
            var c = this.A;
            c && c.depthRange(a, b)
        }
    };
    q.Hf = function() {
        this.qd(0, 1)
    };
    q.zd = function(a, b, c, d) {
        if (this.F[24] != a || this.F[25] != b || this.F[26] != c || this.F[27] != d) {
            bO.V.zd.call(this, a, b, c, d);
            var e = this.A;
            e && e.scissor(a, b, c, d)
        }
    };
    q.Qf = function() {
        this.zd(0, 0, 0, 0)
    };
    q.Ad = function(a, b, c, d) {
        if (this.F[28] != a || this.F[29] != b || this.F[30] != c || this.F[31] != d) {
            bO.V.Ad.call(this, a, b, c, d);
            var e = this.A;
            e && e.viewport(a, b, c, d)
        }
    };
    q.Kg = function() {
        this.Ad(0, 0, 0, 0)
    };
    q.ld = function(a) {
        if (UN(this) != a) {
            bO.V.ld.call(this, a);
            var b = this.A;
            b && b.cullFace(a)
        }
    };
    q.Ef = function() {
        this.ld(1029)
    };
    q.td = function(a) {
        if (VN(this) != a) {
            bO.V.td.call(this, a);
            var b = this.A;
            b && b.frontFace(a)
        }
    };
    q.Kf = function() {
        this.td(2305)
    };
    q.vd = function(a) {
        if (WN(this) != a) {
            bO.V.vd.call(this, a);
            var b = this.A;
            b && b.lineWidth(a)
        }
    };
    q.Mf = function() {
        this.vd(1)
    };
    q.wd = function(a, b) {
        if (!(0 < this.D[148]) || this.B[35] != a || this.B[36] != b) {
            bO.V.wd.call(this, a, b);
            var c = this.A;
            c && c.polygonOffset(a, b)
        }
    };
    q.Nf = function() {
        this.wd(0, 0)
    };
    q.Dc = function(a, b) {
        if (XN(this, a) != b) {
            bO.V.Dc.call(this, a, b);
            var c = this.A;
            c && (b ? c.enableVertexAttribArray(a) : c.disableVertexAttribArray(a))
        }
    };
    q.Tf = function(a) {
        this.Dc(a, !1)
    };
    q.jb = function() {
        return bO.V.jb.call(this)
    };
    q.Sb = function(a) {
        if (this.jb() != a) {
            LN.prototype.Sb.call(this, a);
            var b = this.A;
            b && b.activeTexture(a)
        }
    };
    q.sf = function() {
        this.Sb(33984)
    };
    q.hb = function(a, b) {
        if (ZN(this, a) != b) {
            bO.V.hb.call(this, a, b);
            var c = this.A;
            c && c.pixelStorei(a, b)
        }
    };
    q.zb = function(a) {
        switch (a) {
            case 3317:
            case 3333:
                this.hb(a, 4);
                break;
            case 37440:
            case 37441:
                this.hb(a, 0);
                break;
            default:
                this.hb(a, 37444)
        }
    };
    q.ud = function(a, b) {
        if ($N(this) != b) {
            bO.V.ud.call(this, a, b);
            var c = this.A;
            c && c.hint(a, b)
        }
    };
    q.Lf = function(a) {
        this.ud(a, 4352)
    };

    function cO(a, b, c) {
        this.B = new Jz;
        this.A = a;
        this.K = this.L = 0;
        this.N = 1;
        this.I = this.U = this.H = this.P = 0;
        this.D = [];
        this.G = [];
        this.F = [];
        this.X = E(this.Be, this, this.G);
        this.O = E(this.Be, this, this.D);
        E(this.Be, this, this.F);
        dO(this);
        var d = this;
        gK(function(a) {
            d.N = a ? 1 : .5;
            dO(d)
        }, c, b)
    }
    H(cO, cA);
    q = cO.prototype;
    q.ea = function() {
        this.clear();
        eO(this);
        cO.V.ea.call(this)
    };
    q.contains = function(a) {
        return this.B.contains(a)
    };

    function Yz(a, b) {
        a = a.B;
        (b = a.D[b]) && b.A && ((b.A.next = b.next) ? b.next.A = b.A : a.F = b.A, b.A = null, b.next = a.B, a.B.A = b, a.B = b)
    }
    q.clear = function() {
        this.B.clear()
    };
    q.remove = function(a) {
        this.B.remove(a)
    };

    function KK(a, b, c) {
        return a.B.add(b, a.X, c, 1)
    }

    function Wz(a, b, c) {
        return a.B.add(null, b, c, void 0)
    }
    q.Aa = function(a) {
        return this.B.get(a)
    };
    q.createTexture = function(a) {
        var b = this.A,
            c = b.createTexture();
        b.bindTexture(3553, c);
        b.texParameteri(3553, 10241, a);
        b.texParameteri(3553, 10240, a);
        b.texParameteri(3553, 10242, 33071);
        b.texParameteri(3553, 10243, 33071);
        return KK(this, c, 0)
    };

    function dO(a) {
        var b = 100 * (.75 * a.K + .25 * a.L),
            b = Math.max(48E6, b * a.N),
            c = Math.max(200, .002 * a.K * a.N);
        a.P = .1 * b;
        a.U = .1 * c;
        a = a.B;
        a.C[0] = y(b) ? b : a.C[0];
        a.C[1] = y(c) ? c : a.C[1];
        Lz(a)
    }
    q.Be = function(a, b, c) {
        var d = this.B.D[b];
        this.H += d && d.B;
        b = this.B.D[b];
        this.I += b && b.C;
        a.push(c);
        (this.H >= this.P || this.I >= this.U) && eO(this)
    };

    function eO(a) {
        for (var b = 0; b < a.D.length; b++) a.A.deleteBuffer(a.D[b]);
        for (b = 0; b < a.G.length; b++) a.A.deleteTexture(a.G[b]);
        for (b = 0; b < a.F.length; b++) a.A.deleteRenderbuffer(a.F[b]);
        a.H = 0;
        a.I = 0;
        a.D.splice(0, a.D.length);
        a.G.splice(0, a.G.length);
        a.F.splice(0, a.F.length)
    };

    function fO(a) {
        Ha.call(this, a)
    }
    H(fO, Ha);
    fO.prototype.name = "LostContextError";

    function gO(a, b) {
        this.F = a.createProgram();
        this.B = a;
        this.M = b;
        this.I = [];
        this.J = !1;
        this.C = !0;
        this.D = [];
        this.K = [];
        this.A = [];
        this.G = {};
        this.H = {}
    }
    q = gO.prototype;
    q.wb = function() {
        this.M.D != this && (this.M.D = this, this.B.useProgram(this.F))
    };
    q.attachShader = function(a) {
        this.I.push(a);
        this.B.attachShader(this.F, a)
    };
    q.detachShader = function(a) {
        tb(this.I, a);
        this.B.detachShader(this.F, a)
    };
    q.getAttachedShaders = h("I");
    q.bindAttribLocation = function(a, b) {
        this.B.bindAttribLocation(this.F, a, b);
        this.H[b] = a
    };
    q.getAttribLocation = function(a) {
        var b = this.H[a];
        void 0 === b && (b = this.B.getAttribLocation(this.F, a), this.H[a] = b);
        return b
    };
    q.deleteProgram = function() {
        this.B.deleteProgram(this.F);
        this.J = !0
    };
    q.getParameter = function(a) {
        return this.B.getProgramParameter(this.F, a)
    };
    q.gg = function() {
        return this.B.getProgramInfoLog(this.F)
    };
    q.Jg = function() {
        return !this.J && this.B.isProgram(this.F)
    };
    q.be = function() {
        this.B.linkProgram(this.F);
        this.C = !1
    };

    function hO(a) {
        a.C = !0;
        a.H = {};
        a.D = [];
        a.A = [];
        a.G = {};
        for (var b = a.B.getProgramParameter(a.F, 35718), c = 0, d, e = 0; e < b; ++e) {
            var f = a.B.getActiveUniform(a.F, e);
            if (0 <= f.name.indexOf("[")) {
                var g = f.name.substr(0, f.name.indexOf("[")),
                    k = f.size;
                a.G[g] = c;
                for (var l = 0; l < k; ++l) {
                    d = c++;
                    var m = g + "[" + l + "]";
                    a.G[m] = d;
                    a.K[d] = k - l;
                    a.D[d] = a.B.getUniformLocation(a.F, m);
                    a.A[d] = iO(f.type)
                }
            } else d = c++, a.G[f.name] = d, a.K[d] = 0, a.D[d] = a.B.getUniformLocation(a.F, f.name), a.A[d] = iO(f.type)
        }
    }
    q.Qh = function() {
        this.B.validateProgram(this.F)
    };
    q.getActiveAttrib = function(a) {
        return this.B.getActiveAttrib(this.F, a)
    };
    q.getActiveUniform = function(a) {
        return this.B.getActiveUniform(this.F, a)
    };
    q.getUniform = function(a) {
        this.C || hO(this);
        return -1 == a ? null : this.A[a]
    };
    q.getUniformLocation = function(a) {
        this.C || hO(this);
        return void 0 !== this.G[a] ? this.G[a] : -1
    };
    q.uh = function(a, b) {
        this.C || hO(this);
        var c = this.D,
            d = this.A,
            e = this.B; - 1 != a && b != d[a] && (d[a] = b, e.uniform1f(c[a], b))
    };
    q.yh = function(a, b, c) {
        this.C || hO(this);
        var d = this.D,
            e = this.A,
            f = this.B; - 1 != a && (e = e[a], b != e[0] || c != e[1]) && (e[0] = b, e[1] = c, f.uniform2f(d[a], b, c))
    };
    q.Ch = function(a, b, c, d) {
        this.C || hO(this);
        var e = this.D,
            f = this.A,
            g = this.B; - 1 != a && (f = f[a], b != f[0] || c != f[1] || d != f[2]) && (f[0] = b, f[1] = c, f[2] = d, g.uniform3f(e[a], b, c, d))
    };
    q.Gh = function(a, b, c, d, e) {
        this.C || hO(this);
        var f = this.D,
            g = this.A,
            k = this.B; - 1 != a && (g = g[a], b != g[0] || c != g[1] || d != g[2] || e != g[3]) && (g[0] = b, g[1] = c, g[2] = d, g[3] = e, k.uniform4f(f[a], b, c, d, e))
    };
    q.wh = function(a, b) {
        this.C || hO(this);
        var c = this.D,
            d = this.A,
            e = this.B;
        if (-1 != a) {
            var f = b;
            "boolean" == typeof d[a] && (f = !!b);
            f != d[a] && (d[a] = f, e.uniform1i(c[a], b))
        }
    };
    q.Ah = function(a, b, c) {
        this.C || hO(this);
        var d = this.D,
            e = this.A,
            f = this.B;
        if (-1 != a) {
            var e = e[a],
                g = b,
                k = c;
            e instanceof Array && (g = !!b, k = !!c);
            if (g != e[0] || k != e[1]) e[0] = g, e[1] = k, f.uniform2i(d[a], b, c)
        }
    };
    q.Eh = function(a, b, c, d) {
        this.C || hO(this);
        var e = this.D,
            f = this.A,
            g = this.B;
        if (-1 != a) {
            var f = f[a],
                k = b,
                l = c,
                m = d;
            f instanceof Array && (k = !!b, l = !!c, m = !!d);
            if (k != f[0] || l != f[1] || m != f[2]) f[0] = k, f[1] = l, f[2] = m, g.uniform3i(e[a], b, c, d)
        }
    };
    q.Ih = function(a, b, c, d, e) {
        this.C || hO(this);
        var f = this.D,
            g = this.A,
            k = this.B;
        if (-1 != a) {
            var g = g[a],
                l = b,
                m = c,
                n = d,
                p = e;
            g instanceof Array && (l = !!b, m = !!c, n = !!d, p = !!e);
            if (l != g[0] || m != g[1] || n != g[2] || p != g[3]) g[0] = l, g[1] = m, g[2] = n, g[3] = p, k.uniform4i(f[a], b, c, d, e)
        }
    };
    q.vh = function(a, b) {
        this.C || hO(this);
        if (-1 != a) {
            var c = !1,
                d;
            for (d = 0; !c && d < b.length; ++d) c = b[d] != this.A[a + d];
            if (c) {
                for (d = 0; d < b.length; ++d) this.A[a + d] = b[d];
                this.B.uniform1fv(this.D[a], b)
            }
        }
    };
    q.zh = function(a, b) {
        this.C || hO(this);
        if (-1 != a) {
            var c = !1,
                d;
            for (d = 0; !c && d < b.length / 2; ++d) c = b[2 * d] != this.A[a + d][0] || b[2 * d + 1] != this.A[a + d][1];
            if (c) {
                for (d = 0; d < b.length / 2; ++d) this.A[a + d][0] = b[2 * d], this.A[a + d][1] = b[2 * d + 1];
                this.B.uniform2fv(this.D[a], b)
            }
        }
    };
    q.Dh = function(a, b) {
        this.C || hO(this);
        if (-1 != a) {
            var c = !1,
                d;
            for (d = 0; !c && d < b.length / 3; ++d) c = b[3 * d] != this.A[a + d][0] || b[3 * d + 1] != this.A[a + d][1] || b[3 * d + 2] != this.A[a + d][2];
            if (c) {
                for (d = 0; d < b.length / 3; ++d) this.A[a + d][0] = b[3 * d], this.A[a + d][1] = b[3 * d + 1], this.A[a + d][2] = b[3 * d + 2];
                this.B.uniform3fv(this.D[a], b)
            }
        }
    };
    q.Hh = function(a, b) {
        this.C || hO(this);
        if (-1 != a) {
            var c = !1,
                d;
            for (d = 0; !c && d < b.length / 4; ++d) c = b[4 * d] != this.A[a + d][0] || b[4 * d + 1] != this.A[a + d][1] || b[4 * d + 2] != this.A[a + d][2] || b[4 * d + 3] != this.A[a + d][3];
            if (c) {
                for (d = 0; d < b.length / 4; ++d) this.A[a + d][0] = b[4 * d], this.A[a + d][1] = b[4 * d + 1], this.A[a + d][2] = b[4 * d + 2], this.A[a + d][3] = b[4 * d + 3];
                this.B.uniform4fv(this.D[a], b)
            }
        }
    };
    q.xh = function(a, b) {
        this.C || hO(this);
        if (-1 != a) {
            var c = "boolean" == typeof this.A[a],
                d = !1,
                e;
            for (e = 0; !d && e < b.length; ++e) d = (c ? !!b[e] : b[e]) != this.A[a + e];
            if (d) {
                for (e = 0; e < b.length; ++e) this.A[a + e] = c ? !!b[e] : b[e];
                this.B.uniform1iv(this.D[a], b)
            }
        }
    };
    q.Bh = function(a, b) {
        this.C || hO(this);
        if (-1 != a) {
            var c = this.A[a] instanceof Array,
                d = !1,
                e;
            for (e = 0; !d && e < b.length / 2; ++e) d = c ? !!b[2 * e + 1] : b[2 * e + 1], d = (c ? !!b[2 * e] : b[2 * e]) != this.A[a + e][0] || d != this.A[a + e][1];
            if (d) {
                for (e = 0; e < b.length / 2; ++e) this.A[a + e][0] = c ? !!b[2 * e] : b[2 * e], this.A[a + e][1] = c ? !!b[2 * e + 1] : b[2 * e + 1];
                this.B.uniform2iv(this.D[a], b)
            }
        }
    };
    q.Fh = function(a, b) {
        this.C || hO(this);
        if (-1 != a) {
            var c = this.A[a] instanceof Array,
                d = !1,
                e;
            for (e = 0; !d && e < b.length / 3; ++e) var d = c ? !!b[3 * e + 1] : b[3 * e + 1],
                f = c ? !!b[3 * e + 2] : b[3 * e + 2],
                d = (c ? !!b[3 * e] : b[3 * e]) != this.A[a + e][0] || d != this.A[a + e][1] || f != this.A[a + e][2];
            if (d) {
                for (e = 0; e < b.length / 3; ++e) this.A[a + e][0] = c ? !!b[3 * e] : b[3 * e], this.A[a + e][1] = c ? !!b[3 * e + 1] : b[3 * e + 1], this.A[a + e][2] = c ? !!b[3 * e + 2] : b[3 * e + 2];
                this.B.uniform3iv(this.D[a], b)
            }
        }
    };
    q.Jh = function(a, b) {
        this.C || hO(this);
        if (-1 != a) {
            var c = this.A[a] instanceof Array,
                d = !1,
                e;
            for (e = 0; !d && e < b.length / 4; ++e) var d = c ? !!b[4 * e + 1] : b[4 * e + 1],
                f = c ? !!b[4 * e + 2] : b[4 * e + 2],
                g = c ? !!b[4 * e + 3] : b[4 * e + 3],
                d = (c ? !!b[4 * e] : b[4 * e]) != this.A[a + e][0] || d != this.A[a + e][1] || f != this.A[a + e][2] || g != this.A[a + e][3];
            if (d) {
                for (e = 0; e < b.length / 4; ++e) this.A[a + e][0] = c ? !!b[4 * e] : b[4 * e], this.A[a + e][1] = c ? !!b[4 * e + 1] : b[4 * e + 1], this.A[a + e][2] = c ? !!b[4 * e + 2] : b[4 * e + 2], this.A[a + e][3] = c ? !!b[4 * e + 3] : b[4 * e + 3];
                this.B.uniform4iv(this.D[a], b)
            }
        }
    };
    q.Kh = function(a, b, c) {
        this.C || hO(this);
        if (-1 != a) {
            b = !1;
            var d;
            for (d = 0; !b && d < c.length / 4; ++d) b = c[4 * d] != this.A[a + d][0] || c[4 * d + 1] != this.A[a + d][1] || c[4 * d + 2] != this.A[a + d][2] || c[4 * d + 3] != this.A[a + d][3];
            if (b) {
                for (d = 0; d < c.length / 4; ++d) this.A[a + d][0] = c[4 * d], this.A[a + d][1] = c[4 * d + 1], this.A[a + d][2] = c[4 * d + 2], this.A[a + d][3] = c[4 * d + 3];
                this.B.uniformMatrix2fv(this.D[a], !1, c)
            }
        }
    };
    q.Lh = function(a, b, c) {
        this.C || hO(this);
        if (-1 != a) {
            var d = !1,
                e;
            for (b = 0; !d && b < c.length / 9; ++b) d = 9 * b, e = this.A[a + b], d = c[d] != e[0] || c[d + 1] != e[1] || c[d + 2] != e[2] || c[d + 3] != e[3] || c[d + 4] != e[4] || c[d + 5] != e[5] || c[d + 6] != e[6] || c[d + 7] != e[7] || c[d + 8] != e[8];
            if (d) {
                for (b = 0; b < c.length / 9; ++b) {
                    e = this.A[a + b];
                    for (var d = 9 * b, f = 0; 9 > f; ++f) e[f] = c[d + f]
                }
                this.B.uniformMatrix3fv(this.D[a], !1, c)
            }
        }
    };
    q.Mh = function(a, b, c) {
        this.C || hO(this);
        if (-1 != a) {
            var d = !1,
                e;
            for (b = 0; !d && b < c.length / 16; ++b) d = 16 * b, e = this.A[a + b], d = c[d] != e[0] || c[d + 1] != e[1] || c[d + 2] != e[2] || c[d + 3] != e[3] || c[d + 4] != e[4] || c[d + 5] != e[5] || c[d + 6] != e[6] || c[d + 7] != e[7] || c[d + 8] != e[8] || c[d + 9] != e[9] || c[d + 10] != e[10] || c[d + 11] != e[11] || c[d + 12] != e[12] || c[d + 13] != e[13] || c[d + 14] != e[14] || c[d + 15] != e[15];
            if (d) {
                for (b = 0; b < c.length / 16; ++b) {
                    e = this.A[a + b];
                    for (var d = 16 * b, f = 0; 16 > f; ++f) e[f] = c[d + f]
                }
                this.B.uniformMatrix4fv(this.D[a], !1, c)
            }
        }
    };

    function iO(a) {
        switch (a) {
            case 35670:
                return !1;
            case 5124:
            case 5126:
            case 35678:
            case 35680:
                return 0;
            case 35664:
                return new Float32Array(2);
            case 35667:
                return new Int32Array(2);
            case 35671:
                return [!1, !1];
            case 35665:
                return new Float32Array(3);
            case 35668:
                return new Int32Array(3);
            case 35672:
                return [!1, !1, !1];
            case 35666:
                return new Float32Array(4);
            case 35669:
                return new Int32Array(4);
            case 35673:
                return [!1, !1, !1, !1];
            case 35674:
                return new Float32Array(4);
            case 35675:
                return new Float32Array(9);
            case 35676:
                return new Float32Array(16)
        }
        return null
    };

    function jO(a, b) {
        il.call(this);
        this.ta = kO++;
        this.C = a;
        this.A = b;
        this.state = new bO(this.A);
        this.F = new KN(this.A, this.state);
        this.L = new wq(this);
        Ck(this, Fa(Dk, this.L));
        this.B = new cO(this, void 0, this.L);
        Ck(this, Fa(Dk, this.B));
        this.D = null;
        this.H = this.J = this.G = void 0;
        this.getParameter(3379);
        this.getParameter(34076);
        this.I = void 0;
        yq(this.L, a, "webglcontextlost", this.Hk, !1, this);
        yq(this.L, a, "webglcontextrestored", this.Ok, !1, this);
        lO(this)
    }
    H(jO, il);
    var kO = 0;
    q = jO.prototype;
    q.za = h("ta");
    q.ea = function() {
        this.D = null;
        this.A.useProgram(null);
        jO.V.ea.call(this)
    };

    function lO(a) {
        var b = (a.A.drawingBufferWidth || a.C.A.width) * (a.A.drawingBufferHeight || a.C.A.height),
            c = a.C.C;
        a = a.B;
        c = b / (c * c);
        if (b != a.L || c != a.K) a.L = b, a.K = c, dO(a)
    }

    function GK(a, b) {
        var c = a.state.jb() - 33984;
        3553 == b ? (a = a.F.B[c], 3553 != a.A && (a.A = 3553)) : (a = a.F.C[c], 34067 != a.A && (a.A = 34067), 34067 != b && (a.L = b));
        return a
    }

    function HK(a, b, c, d, e, f, g, k) {
        a = GK(a, b);
        EN(a, d, e, f, g, c);
        b = FN(a);
        a.bind();
        GN(a, d, f, g);
        a.B.texImage2D(b, c, f, d, e, 0, f, g, k);
        a.F.zb(3317)
    }
    q.texImage2D = function(a, b, c, d, e, f, g, k, l) {
        g ? HK(this, a, b, d, e, g, k, l) : FK(GK(this, a), f, d, e, b)
    };
    q.texSubImage2D = function(a, b, c, d, e, f, g, k, l) {
        if (k) {
            a = GK(this, a);
            var m = FN(a);
            a.bind();
            a.B.texSubImage2D(m, b, c, d, e, f, g, k, l)
        } else k = GK(this, a), l = FN(k), k.bind(), GN(k, g.width, e, f), k.B.texSubImage2D(l, b, c, d, e, f, g), k.F.zb(3317)
    };
    q.compressedTexImage2D = function(a, b, c, d, e, f, g) {
        a = GK(this, a);
        EN(a, d, e, c, 0, b);
        f = FN(a);
        a.bind();
        a.B.compressedTexImage2D(f, b, c, d, e, 0, g)
    };
    q.compressedTexSubImage2D = function(a, b, c, d, e, f, g, k) {
        a = GK(this, a);
        var l = FN(a);
        a.bind();
        a.B.compressedTexSubImage2D(l, b, c, d, e, f, g, k)
    };
    q.activeTexture = function(a) {
        this.state.Sb(a)
    };
    q.blendColor = function(a, b, c, d) {
        this.state.ed(a, b, c, d)
    };
    q.blendEquation = function(a) {
        this.state.Bc(a)
    };
    q.blendEquationSeparate = function(a, b) {
        this.state.Bc(a, b)
    };
    q.blendFunc = function(a, b) {
        this.state.Cc(a, b)
    };
    q.blendFuncSeparate = function(a, b, c, d) {
        this.state.Cc(a, b, c, d)
    };
    q.clearColor = function(a, b, c, d) {
        this.state.fd(a, b, c, d)
    };
    q.clearDepth = function(a) {
        this.state.hd(a)
    };
    q.clearStencil = function(a) {
        this.state.jd(a)
    };
    q.colorMask = function(a, b, c, d) {
        this.state.kd(a, b, c, d)
    };
    q.cullFace = function(a) {
        this.state.ld(a)
    };
    q.depthFunc = function(a) {
        this.state.md(a)
    };
    q.depthMask = function(a) {
        this.state.od(a)
    };
    q.depthRange = function(a, b) {
        this.state.qd(a, b)
    };
    q.disable = function(a) {
        this.state.La(a, !1)
    };
    q.disableVertexAttribArray = function(a) {
        this.state.Dc(a, !1)
    };
    q.enable = function(a) {
        this.state.La(a, !0)
    };
    q.enableVertexAttribArray = function(a) {
        this.state.Dc(a, !0)
    };
    q.frontFace = function(a) {
        this.state.td(a)
    };
    q.hint = function(a, b) {
        this.state.ud(a, b)
    };
    q.isEnabled = function(a) {
        return NN(this.state, a)
    };
    q.lineWidth = function(a) {
        this.state.vd(a)
    };
    q.pixelStorei = function(a, b) {
        this.state.hb(a, b)
    };
    q.polygonOffset = function(a, b) {
        this.state.wd(a, b)
    };
    q.sampleCoverage = function(a, b) {
        this.state.yd(a, b)
    };
    q.scissor = function(a, b, c, d) {
        this.state.zd(a, b, c, d)
    };
    q.stencilFunc = function(a, b, c) {
        this.A.stencilFunc(a, b, c)
    };
    q.stencilMask = function(a) {
        this.A.stencilMask(a)
    };
    q.stencilOp = function(a, b, c) {
        this.A.stencilOp(a, b, c)
    };
    q.viewport = function(a, b, c, d) {
        lO(this);
        this.state.Ad(a, b, c, d)
    };
    q.bindBuffer = function(a, b) {
        34962 == a ? this.F.dd(b) : this.F.rd(b)
    };
    q.bindFramebuffer = function(a, b) {
        this.F.sd(b)
    };
    q.bindRenderbuffer = function(a, b) {
        this.F.xd(b)
    };
    q.bindTexture = function(a, b) {
        var c = this.state.jb() - 33984;
        b && (b.A != a && (b.A = a), b.C != c && (b.C = c));
        3553 == a ? this.F.jc(c, b) : this.F.kc(c, b)
    };
    q.attachShader = function(a, b) {
        a.attachShader && a.attachShader(b)
    };
    q.bindAttribLocation = function(a, b, c) {
        a.bindAttribLocation && a.bindAttribLocation(b, c)
    };
    q.createProgram = function() {
        return new gO(this.A, this)
    };
    q.deleteProgram = function(a) {
        a.deleteProgram && a.deleteProgram()
    };
    q.detachShader = function(a, b) {
        a.detachShader && a.detachShader(b)
    };
    q.getActiveAttrib = function(a, b) {
        return a.getActiveAttrib ? a.getActiveAttrib(b) : null
    };
    q.getActiveUniform = function(a, b) {
        return a.getActiveUniform ? a.getActiveUniform(b) : null
    };
    q.getAttachedShaders = function(a) {
        return a.getAttachedShaders ? a.getAttachedShaders() : []
    };
    q.getAttribLocation = function(a, b) {
        return a.getAttribLocation ? a.getAttribLocation(b) : -1
    };
    q.getProgramParameter = function(a, b) {
        return a.getParameter ? a.getParameter(b) : -1
    };
    q.getProgramInfoLog = function(a) {
        return a.gg ? a.gg() : ""
    };
    q.getUniform = function(a, b) {
        return a.getUniform ? a.getUniform(b) : null
    };
    q.getUniformLocation = function(a, b) {
        return a.getUniformLocation ? a.getUniformLocation(b) : -1
    };
    q.isProgram = function(a) {
        return a.Jg ? a.Jg() : !1
    };
    q.linkProgram = function(a) {
        a.be && a.be()
    };
    q.uniform1f = function(a, b) {
        var c = this.D;
        c && c.uh && c.uh(a, b)
    };
    q.uniform1fv = function(a, b) {
        var c = this.D;
        c && c.vh && c.vh(a, b)
    };
    q.uniform1i = function(a, b) {
        var c = this.D;
        c && c.wh && c.wh(a, b)
    };
    q.uniform1iv = function(a, b) {
        var c = this.D;
        c && c.xh && c.xh(a, b)
    };
    q.uniform2f = function(a, b, c) {
        var d = this.D;
        d && d.yh && d.yh(a, b, c)
    };
    q.uniform2fv = function(a, b) {
        var c = this.D;
        c && c.zh && c.zh(a, b)
    };
    q.uniform2i = function(a, b, c) {
        var d = this.D;
        d && d.Ah && d.Ah(a, b, c)
    };
    q.uniform2iv = function(a, b) {
        var c = this.D;
        c && c.Bh && c.Bh(a, b)
    };
    q.uniform3f = function(a, b, c, d) {
        var e = this.D;
        e && e.Ch && e.Ch(a, b, c, d)
    };
    q.uniform3fv = function(a, b) {
        var c = this.D;
        c && c.Dh && c.Dh(a, b)
    };
    q.uniform3i = function(a, b, c, d) {
        var e = this.D;
        e && e.Eh && e.Eh(a, b, c, d)
    };
    q.uniform3iv = function(a, b) {
        var c = this.D;
        c && c.Fh && c.Fh(a, b)
    };
    q.uniform4f = function(a, b, c, d, e) {
        var f = this.D;
        f && f.Gh && f.Gh(a, b, c, d, e)
    };
    q.uniform4fv = function(a, b) {
        var c = this.D;
        c && c.Hh && c.Hh(a, b)
    };
    q.uniform4i = function(a, b, c, d, e) {
        var f = this.D;
        f && f.Ih && f.Ih(a, b, c, d, e)
    };
    q.uniform4iv = function(a, b) {
        var c = this.D;
        c && c.Jh && c.Jh(a, b)
    };
    q.uniformMatrix2fv = function(a, b, c) {
        (b = this.D) && b.Kh && b.Kh(a, 0, c)
    };
    q.uniformMatrix3fv = function(a, b, c) {
        (b = this.D) && b.Lh && b.Lh(a, 0, c)
    };
    q.uniformMatrix4fv = function(a, b, c) {
        (b = this.D) && b.Mh && b.Mh(a, 0, c)
    };
    q.useProgram = function(a) {
        a.wb && a.wb()
    };
    q.validateProgram = function(a) {
        a.Qh && a.Qh()
    };
    q.getContextAttributes = function() {
        return this.A.getContextAttributes()
    };
    q.isContextLost = function() {
        return this.A.isContextLost()
    };
    q.getSupportedExtensions = function() {
        var a = this.A.getSupportedExtensions();
        if (!a && this.isContextLost()) throw new fO("getSupportedExtensions");
        return a
    };
    q.getExtension = function(a) {
        return this.A.getExtension(a)
    };
    var mO = ["WEBGL_compressed_texture_s3tc", "WEBKIT_WEBGL_compressed_texture_s3tc", "MOZ_WEBGL_compressed_texture_s3tc"];

    function nO(a) {
        if (y(a.G)) return !!a.G;
        if (Jd && !ed(30))
            for (var b = a.getSupportedExtensions(), c = mO, d = 0; d < b.length; d++)
                for (var e = 0; e < c.length; e++) {
                    if (b[d] == c[e] && (a.G = a.getExtension(c[e]), a.G)) return !0
                } else
                    for (c = mO, e = 0; e < c.length; e++)
                        if (a.G = a.getExtension(c[e]), a.G) return !0;
        a.G = null;
        return !1
    }
    q = jO.prototype;
    q.bufferData = function(a, b, c) {
        this.A.bufferData(a, b, c)
    };
    q.bufferSubData = function(a, b, c) {
        this.A.bufferSubData(a, b, c)
    };
    q.checkFramebufferStatus = function(a) {
        return this.A.checkFramebufferStatus(a)
    };
    q.clear = function(a) {
        this.A.clear(a)
    };
    q.compileShader = function(a) {
        this.A.compileShader(a)
    };
    q.copyTexImage2D = function(a, b, c, d, e, f, g) {
        a = GK(this, a);
        EN(a, f, g, c, 5121, b);
        var k = FN(a);
        a.bind();
        a.B.copyTexImage2D(k, b, c, d, e, f, g, 0)
    };
    q.copyTexSubImage2D = function(a, b, c, d, e, f, g, k) {
        a = GK(this, a);
        var l = FN(a);
        a.bind();
        a.B.copyTexSubImage2D(l, b, c, d, e, f, g, k)
    };
    q.createBuffer = function() {
        return this.A.createBuffer()
    };
    q.createFramebuffer = function() {
        return this.A.createFramebuffer()
    };
    q.createRenderbuffer = function() {
        return this.A.createRenderbuffer()
    };
    q.createShader = function(a) {
        return this.A.createShader(a)
    };
    q.createTexture = function() {
        return new zN(this.A, this.state, this.F)
    };
    q.deleteBuffer = function(a) {
        this.A.deleteBuffer(a)
    };
    q.deleteFramebuffer = function(a) {
        this.A.deleteFramebuffer(a)
    };
    q.deleteRenderbuffer = function(a) {
        this.A.deleteRenderbuffer(a)
    };
    q.deleteShader = function(a) {
        this.A.deleteShader(a)
    };
    q.deleteTexture = function(a) {
        a && a.deleteTexture()
    };
    q.drawArrays = function(a, b, c) {
        this.A.drawArrays(a, b, c)
    };
    q.drawElements = function(a, b, c, d) {
        this.A.drawElements(a, b, c, d)
    };
    q.finish = function() {
        this.A.finish()
    };
    q.flush = function() {
        this.A.flush()
    };
    q.framebufferRenderbuffer = function(a, b, c, d) {
        this.A.framebufferRenderbuffer(a, b, c, d)
    };
    q.framebufferTexture2D = function(a, b, c, d, e) {
        this.A.framebufferTexture2D(a, b, c, d.G, e)
    };
    q.generateMipmap = function(a) {
        GK(this, a).generateMipmap()
    };
    q.getBufferParameter = function(a, b) {
        a = this.A.getBufferParameter(a, b);
        if (null === a && this.isContextLost()) throw new fO("getBufferParameter");
        return a
    };
    q.getParameter = function(a) {
        switch (a) {
            case 32873:
                return this.F.B[this.state.jb() - 33984];
            case 34068:
                return this.F.C[this.state.jb() - 33984];
            case 35725:
                return this.D
        }
        a = this.A.getParameter(a);
        if (null === a && this.isContextLost()) throw new fO("getParameter");
        return a
    };
    q.getError = function() {
        return this.A.getError()
    };
    q.getFramebufferAttachmentParameter = function(a, b, c) {
        a = this.A.getFramebufferAttachmentParameter(a, b, c);
        if (null === a && this.isContextLost()) throw new fO("getFramebufferAttachmentParameter");
        return a
    };
    q.getRenderbufferParameter = function(a, b) {
        a = this.A.getRenderbufferParameter(a, b);
        if (null === a && this.isContextLost()) throw new fO("getRenderbufferParameter");
        return a
    };
    q.getShaderParameter = function(a, b) {
        a = this.A.getShaderParameter(a, b);
        if (null === a && this.isContextLost()) throw new fO("getShaderParameter");
        return a
    };
    q.getShaderInfoLog = function(a) {
        return this.A.getShaderInfoLog(a)
    };
    q.getShaderSource = function(a) {
        return this.A.getShaderSource(a)
    };
    q.getTexParameter = function(a, b) {
        a = GK(this, a);
        switch (b) {
            case 10241:
                return a.K;
            case 10240:
                return a.I;
            case 10242:
                return a.J;
            case 10243:
                return a.M
        }
        return 0
    };
    q.getVertexAttrib = function(a, b) {
        a = this.A.getVertexAttrib(a, b);
        if (null === a && this.isContextLost()) throw new fO("getVertexAttrib");
        return a
    };
    q.getVertexAttribOffset = function(a, b) {
        return this.A.getVertexAttribOffset(a, b)
    };
    q.isBuffer = function(a) {
        return this.A.isBuffer(a)
    };
    q.isFramebuffer = function(a) {
        return this.A.isFramebuffer(a)
    };
    q.isRenderbuffer = function(a) {
        return this.A.isRenderbuffer(a)
    };
    q.isShader = function(a) {
        return this.A.isShader(a)
    };
    q.isTexture = function(a) {
        return !a.N && this.A.isTexture(a.G)
    };
    q.readPixels = function(a, b, c, d, e, f, g) {
        this.A.readPixels(a, b, c, d, e, f, g)
    };
    q.renderbufferStorage = function(a, b, c, d) {
        this.A.renderbufferStorage(a, b, c, d)
    };
    q.shaderSource = function(a, b) {
        this.A.shaderSource(a, b)
    };
    q.texParameterf = function(a, b, c) {
        a = GK(this, a);
        switch (b) {
            case 10241:
                CN(a, c);
                break;
            case 10240:
                DN(a, c);
                break;
            case 10242:
                AN(a, c);
                break;
            case 10243:
                BN(a, c)
        }
    };
    q.texParameteri = function(a, b, c) {
        a = GK(this, a);
        switch (b) {
            case 10241:
                CN(a, c);
                break;
            case 10240:
                DN(a, c);
                break;
            case 10242:
                AN(a, c);
                break;
            case 10243:
                BN(a, c)
        }
    };
    q.vertexAttrib1f = function(a, b) {
        this.A.vertexAttrib1f(a, b)
    };
    q.vertexAttrib1fv = function(a, b) {
        this.A.vertexAttrib1fv(a, b)
    };
    q.vertexAttrib2f = function(a, b, c) {
        this.A.vertexAttrib2f(a, b, c)
    };
    q.vertexAttrib2fv = function(a, b) {
        this.A.vertexAttrib2fv(a, b)
    };
    q.vertexAttrib3f = function(a, b, c, d) {
        this.A.vertexAttrib3f(a, b, c, d)
    };
    q.vertexAttrib3fv = function(a, b) {
        this.A.vertexAttrib3fv(a, b)
    };
    q.vertexAttrib4f = function(a, b, c, d, e) {
        this.A.vertexAttrib4f(a, b, c, d, e)
    };
    q.vertexAttrib4fv = function(a, b) {
        this.A.vertexAttrib4fv(a, b)
    };
    q.vertexAttribPointer = function(a, b, c, d, e, f) {
        this.A.vertexAttribPointer(a, b, c, d, e, f)
    };

    function oO(a) {
        a.B.clear();
        a.D = null;
        a.F.clear();
        a.state.clear()
    }
    q.Hk = function(a) {
        a.preventDefault();
        oO(this);
        F();
        this.dispatchEvent("webglcontextlost")
    };
    q.Ok = function() {
        oO(this);
        if (this.G && (this.G = void 0, !nO(this))) throw Error("Lost compressed textures extension.");
        if (this.J) {
            this.J = void 0;
            var a;
            if (y(this.J)) a = !!this.J;
            else {
                if (a = this.getExtension("OES_texture_float")) {
                    this.getExtension("OES_texture_float_linear");
                    this.getExtension("WEBGL_color_buffer_float");
                    for (var b = 0; 8 > b; ++b) this.disableVertexAttribArray(b);
                    this.disable(3089);
                    this.disable(2960);
                    this.disable(2929);
                    this.disable(3042);
                    this.disable(2884);
                    b = this.createShader(35633);
                    this.shaderSource(b,
                        "attribute vec4 vertexClip;\nvoid main() {\n  gl_Position = vec4(vertexClip.xy, 0.0, 1.0);\n}");
                    this.compileShader(b);
                    var c = this.createShader(35632);
                    this.shaderSource(c, "precision highp float;\nuniform sampler2D sampler;\nuniform float mode;\nvoid main() {\n  if (mode == 0.0) {\n    gl_FragColor = floor(gl_FragCoord.xyxy);\n  } else {\n    gl_FragColor = texture2D(sampler, vec2(0.5));\n  }\n}\n");
                    this.compileShader(c);
                    var d = this.createProgram();
                    d.attachShader(b);
                    d.attachShader(c);
                    d.be();
                    d.wb();
                    var e =
                        this.createBuffer();
                    this.bindBuffer(34962, e);
                    this.bufferData(34962, new Float32Array([-1, -1, 1, 1, 1, -1, 1, 1, -1, 1, 1, 1, 1, 1, 1, 1]), 35044);
                    this.enableVertexAttribArray(d.getAttribLocation("vertexClip"));
                    this.vertexAttribPointer(d.getAttribLocation("vertexClip"), 4, 5126, !1, 0, 0);
                    this.activeTexture(33984);
                    var f = this.createTexture();
                    this.bindTexture(3553, f);
                    this.texParameteri(3553, 10241, 9729);
                    this.texParameteri(3553, 10240, 9729);
                    this.texParameteri(3553, 10242, 33071);
                    this.texParameteri(3553, 10243, 33071);
                    HK(this, 3553,
                        0, 2, 2, 6408, 5126, null);
                    this.bindTexture(3553, null);
                    var g = this.createFramebuffer();
                    this.bindFramebuffer(36160, g);
                    this.framebufferTexture2D(36160, 36064, 3553, f, 0);
                    this.uniform1f(d.getUniformLocation("mode"), 0);
                    this.uniform1i(d.getUniformLocation("sampler"), 0);
                    this.viewport(0, 0, 2, 2);
                    this.drawArrays(5, 0, 4);
                    this.bindFramebuffer(36160, null);
                    this.uniform1f(d.getUniformLocation("mode"), 1);
                    this.drawArrays(5, 0, 4);
                    var k = new Uint8Array([0, 0, 0, 0]);
                    this.readPixels(0, 0, 1, 1, 6408, 5121, k);
                    this.disableVertexAttribArray(d.getAttribLocation("vertexClip"));
                    this.deleteBuffer(e);
                    this.deleteTexture(f);
                    this.deleteFramebuffer(g);
                    this.detachShader(d, b);
                    this.deleteShader(b);
                    this.detachShader(d, c);
                    this.deleteShader(c);
                    this.deleteProgram(d);
                    if (2 < Math.abs(k[0] - 127) || 2 < Math.abs(k[1] - 127) || 2 < Math.abs(k[2] - 127)) a = null
                }
                this.J = a;
                a = !!a
            }
            if (!a) throw Error("Lost texture float extension.");
        }
        if (this.H && (this.H = void 0, y(this.H) || (this.H = this.getExtension("WEBGL_depth_texture")), !this.H)) throw Error("Lost depth texture extension.");
        y(this.I) && (this.I = void 0, y(this.I) || (Jd &&
            Xc && !(0 <= db(iE, "30")) || Fd && Xc && !(0 <= db(iE, "27")) ? this.I = null : this.I = this.getExtension("ANGLE_instanced_arrays")));
        F();
        this.dispatchEvent("webglcontextrestored")
    };

    function pO(a, b) {
        b = lE(a.A, b || new gE);
        if (!b) throw Error("Could not find a 3d context, error: " + kE);
        return new jO(a, b)
    };

    function qO() {};

    function rO() {
        this.A = []
    }
    rO.prototype.handleEvent = function(a, b) {
        for (var c = this.A, d = 0, e = c.length; d < e; d++) c[d](a, b)
    };

    function sO() {
        this.C = this.A = null;
        this.B = []
    }
    var tO = new Do(Mo());

    function uO(a, b, c) {
        var d = c ? c : new wo(tO, "buff_pass_logger");
        b.get(function(b) {
            a.A = b;
            b = 0;
            for (var e = a.B.length; b < e; b++) {
                var g = a.B[b];
                g.Jk(a.A).apply(a.A, g.oc);
                g.oa && g.oa.done("bpl-branch")
            }
            a.B.length = 0;
            a.C = null;
            c || d.done("main-actionflow-branch")
        }, d)
    }

    function vO(a, b, c) {
        a.A ? b(a.A).apply(a.A, c) : (a.B.push({
            Jk: b,
            oc: c,
            oa: null
        }), a.C && uO(a, a.C))
    }
    sO.prototype.F = function(a, b, c, d, e, f, g) {
        vO(this, function(a) {
            return a.F
        }, arguments)
    };
    sO.prototype.G = function(a, b, c, d) {
        vO(this, function(a) {
            return a.G
        }, arguments)
    };
    sO.prototype.D = function(a, b, c) {
        vO(this, function(a) {
            return a.D
        }, arguments)
    };

    function wO(a) {
        this.data = a || []
    }
    H(wO, Wg);
    ra(wO);

    function xO(a) {
        this.data = a || []
    }
    H(xO, J);
    xO.prototype.A = function() {
        return K(this, 89)
    };

    function yO() {
        this.A = {}
    }
    yO.prototype.load = function(a, b, c) {
        var d;
        0 === Xs(a) ? d = 0 : 3 === Xs(a) ? d = 3 : 1 === Xs(a) ? d = 1 : 2 === Xs(a) && (d = 2);
        this.A[d].load(a, b, c)
    };

    function zO(a) {
        this.data = a || []
    }
    H(zO, J);

    function AO(a, b, c, d) {
        V.call(this, "CPW", wb(arguments))
    }
    H(AO, V);

    function BO() {
        this.B = this.canvas = null
    };

    function CO(a, b) {
        BO.call(this);
        this.A = a;
        this.canvas = b
    }
    H(CO, BO);
    CO.prototype.getContext = function(a, b) {
        a(this.A, b)
    };

    function DO(a, b, c, d, e, f) {
        V.call(this, "SCIR", wb(arguments))
    }
    H(DO, V);

    function EO(a, b, c, d, e, f, g) {
        this.F = a;
        this.C = b;
        this.H = c;
        this.B = new iK(d, void 0, Ic(a, 55));
        this.G = e;
        this.D = f;
        this.I = g;
        this.A = {}
    }
    EO.prototype.load = function(a, b, c) {
        var d, e, f = this.A[a];
        f ? f.C(c, b) : ("pa" === a ? (b.ua("pard0"), d = "pard1", e = new Iz(this.B, this.C, this.F, this.H, this.G, this.D), f = E(function(a, b) {
            this.I && By(a.$, this.I.A, b)
        }, this), e.C(f, b)) : "ph" === a && (b.ua("phrd0"), d = "phrd1", e = new DO(this.B, this.C, this.F, this.H, this.G, this.D)), this.A[a] = e, e.get(function(a, b) {
            b.ua(d);
            c(a, b)
        }, b));
        return new DC
    };

    function FO(a, b, c) {
        var d = a.A.pa;
        d ? d.B() ? b(d.A()) : d.C(function(a) {
            b(a)
        }, c) : a.load("pa", c, function(a) {
            b(a)
        })
    };

    function GO(a, b, c, d, e, f) {
        this.F = a;
        this.I = b;
        this.H = c;
        this.G = d;
        this.D = e;
        this.B = this.A = null;
        this.C = f || C
    }
    GO.prototype.load = function(a, b, c) {
        if (this.A) b(this.A, c), this.C(this.A);
        else {
            var d = this;
            d.B || (d.B = new sx(d.H, d.F, d.I, this.G, d.D));
            d.B.get(function(a, c) {
                d.A = a;
                b(d.A, c);
                d.C(d.A)
            }, c)
        }
    };

    function HO(a, b, c, d) {
        V.call(this, "SCHI", wb(arguments))
    }
    H(HO, V);

    function IO(a, b, c, d) {
        this.B = a;
        this.D = b;
        this.C = c;
        this.F = d;
        this.A = null
    }
    IO.prototype.load = function(a, b, c) {
        this.A || (this.A = new HO(this.D, this.B, this.C, this.F));
        this.A.get(function(a, c) {
            b(a, c)
        }, c)
    };

    function JO(a, b, c, d, e, f, g, k, l) {
        V.call(this, "FP", wb(arguments))
    }
    H(JO, V);

    function KO(a, b, c, d, e) {
        V.call(this, "FLP", wb(arguments))
    }
    H(KO, V);

    function LO(a, b, c, d, e, f) {
        V.call(this, "IMW", wb(arguments))
    }
    H(LO, V);

    function MO(a, b, c, d, e, f, g, k) {
        V.call(this, "LB", wb(arguments))
    }
    H(MO, V);

    function NO(a, b, c, d, e, f, g, k, l) {
        V.call(this, "LOG", wb(arguments))
    }
    H(NO, V);

    function OO(a, b) {
        V.call(this, "VLG", wb(arguments))
    }
    H(OO, V);

    function PO(a) {
        this.D = a
    }
    PO.prototype.B = ca(!0);
    PO.prototype.get = function(a, b) {
        a(this.D, b)
    };
    PO.prototype.A = h("D");
    PO.prototype.C = function(a, b) {
        this.get(a, b)
    };

    function QO() {}
    QO.prototype.report = C;
    QO.prototype.F = C;
    QO.prototype.G = function(a, b) {
        a && w.open(a, b)
    };
    QO.prototype.D = C;

    function RO(a) {
        this.data = a || []
    }
    H(RO, J);
    RO.prototype.Da = function() {
        return new Vs(this.data[0])
    };
    RO.prototype.fa = function() {
        return new le(this.data[1])
    };

    function SO(a, b, c, d, e, f, g, k, l) {
        V.call(this, "RAP", wb(arguments))
    }
    H(SO, V);

    function TO() {
        this.left = this.bottom = this.right = this.top = 0;
        this.A = !0
    };

    function UO() {
        this.B = Gy(WD);
        this.B.listen(this.C, this);
        this.A = Hy(IE)
    }
    UO.prototype.C = function(a) {
        var b = this.A.get();
        b || (b = new TO, this.A.set(b, a));
        b.top = 0;
        b.bottom = this.B.get() || 0;
        b.A = !0;
        b.left = 0;
        b.right = 0;
        Dy(this.A, a)
    };
    UO.prototype.bind = function(a, b) {
        By(this.B, a, b)
    };

    function VO(a, b, c, d) {
        V.call(this, "TTW", wb(arguments))
    }
    H(VO, V);

    function WO(a, b, c, d, e, f, g, k) {
        V.call(this, "TCW", wb(arguments))
    }
    H(WO, V);

    function XO(a) {
        return 2 === Xs(a) || 1 === Xs(a) || 3 === Xs(a)
    }

    function YO(a) {
        switch (a) {
            case 1:
                return new Ex(1);
            case 3:
                return new Ex(3)
        }
        return new Ex(2)
    };

    function ZO(a, b, c, d, e) {
        V.call(this, "VF", wb(arguments))
    }
    H(ZO, V);

    function $O(a, b, c, d, e, f) {
        V.call(this, "VH", wb(arguments))
    }
    H($O, V);

    function aP(a, b, c, d, e, f) {
        V.call(this, "ZMW", wb(arguments))
    }
    H(aP, V);

    function bP(a) {
        this.C = a ? a : window
    }
    bP.prototype.B = function(a) {
        var b = this.C;
        (b.requestAnimationFrame || b.webkitRequestAnimationFrame || b.oRequestAnimationFrame || b.msRequestAnimationFrame || C).call(b, a)
    };
    bP.prototype.A = function() {
        var a = this.C;
        return a.animationStartTime || a.mozAnimationStartTime || a.webkitAnimationStartTime || a.osAnimationStartTime || a.msAnimationStartTime || F()
    };

    function cP(a) {
        this.F = a ? a : new bP;
        this.A = [];
        this.B = [];
        this.C = [];
        this.G = E(this.H, this);
        this.D = !1
    }

    function dP(a, b, c) {
        this.A = a;
        this.D = 1 / b;
        this.B = 0;
        this.G = c;
        this.C = 0;
        this.F = !1
    }

    function nz(a, b, c) {
        var d = y(void 0) ? void 0 : 1;
        0 < d && (c = new dP(b, c, d), c.B = a.F.A(), a.A.push(c), b.B(0), eP(a))
    }

    function eP(a) {
        a.D || (a.D = !0, a.F.B(a.G))
    }
    cP.prototype.H = function(a) {
        this.D = !1;
        var b = [],
            c = this.A;
        this.A = this.B;
        this.B = c;
        var d;
        for (d = 0; d < c.length; d++) {
            var e = c[d],
                f = (a - e.B) * e.D;
            1 <= f ? (e.A.B(1), e.C++, e.C >= e.G ? (b.push(d), e.F = !0) : (e.A.B(0), e.B = a)) : 0 < f && e.A.B(f)
        }
        a = b.length;
        e = c.length;
        for (d = a - 1; 0 <= d; d--) c[b[d]] = c[--e];
        c.length = e;
        a = this.C.length;
        for (d = 0; d < a; d++)
            for (b = this.C[d], f = e - 1; 0 <= f; f--)
                if (c[f].A == b.A) {
                    c[f] = c[--e];
                    break
                }
        c.length = e;
        for (d = this.C.length = 0; d < e; d++) this.A.push(c[d]);
        this.B.length = 0;
        0 < e && eP(this)
    };

    function fP(a) {
        this.D = a;
        this.C = C
    }
    fP.prototype.B = function(a) {
        this.C = a;
        a = this.D;
        a.I.push(this);
        ZD(a.B)
    };
    fP.prototype.A = function() {
        var a = this.D;
        return a.H && a.M ? a.da : F()
    };

    function hN(a, b, c) {
        il.call(this);
        this.L = Po();
        var d = new wo(this.L, "application");
        this.ja = b;
        this.A = a;
        U(wO.hg(), new Wg(a.data[5]));
        this.ha = b.U;
        this.Pb = b.O;
        var e = this.G = new uI,
            f = new zO;
        a = Uh(a);
        f.data[0] = K(a, 11) ? O(a, 11) : "//maps.gstatic.com";
        e = e.F;
        f = oG(f);
        e.add("g-3ZqzcwcZGCQ", f);
        this.D = b.H;
        this.F = b.G;
        this.O = b.Y;
        this.aa = b.$;
        f = Fy();
        this.width = Hy(f);
        this.height = Hy(f);
        this.Zb = !0;
        this.$ = Hy(f);
        this.Xe = Gy(Fy());
        a = Fy();
        this.J = Gy(a, !1);
        this.Xd = Gy(a, !1);
        var e = new xO,
            g = this.A,
            k = Uh(g);
        e.data[0] = !0;
        e.data[76] = Ic(Wh(g),
            4, !0);
        var l;
        for (l = 0; l < S(k, 3); ++l) {
            var m = Lc(k, 3, l);
            Kc(e, 13, m)
        }
        U(new Ee(P(e, 16)), new Ee(g.data[1]));
        e.data[64] = O(k, 8);
        e.data[75] = O(k, 10);
        e.data[78] = Ic(Wh(g), 7);
        for (l = 0; l < S(k, 9); ++l) m = Lc(k, 9, l), Kc(e, 73, m);
        K(g, 14) && (e.data[86] = Ic(g, 14));
        K(g, 15) && (e.data[87] = O(g, 15));
        K(g, 16) && U(new vd(P(e, 89)), new vd(g.data[16]));
        k = b.getContext();
        k.Sa ? e.data[20] = 1 : k.yb ? e.data[20] = 2 : k.A && (e.data[20] = 3);
        e.data[81] = this.ha.id;
        e.data[82] = "viewport";
        this.Ua = Gy(a, Ic(e, 2));
        this.xa = new cP(new fP(this.D));
        l = Rj("div");
        l.style.width =
            "100%";
        l.style.height = "100%";
        g = new cP(new fP(this.D));
        a = new CO(k, b.C);
        a.B = l;
        c || (Ic(this.A, 9, !0) ? (c = new rO, this.F.Qb("visibilitychange", c, c.handleEvent), c = new NO(O(Uh(this.A), 0), this.F, new qO, c, this.O, !0, null, O(new Ld(this.A.data[11]), 2), O(this.A, 6))) : c = new PO(new QO));
        this.I = c;
        c = this.Y = new sO;
        k = this.I;
        c.B.length ? uO(c, k, d) : c.C = k;
        this.Wa = new EO(e, a, this.O, this.D, this.Y, g, this.aa);
        this.Fb = null;
        this.H = {
            Lc: null,
            zoom: null
        };
        this.C = this.X = null;
        c = b.D;
        g = this.Wa;
        k = this.Y;
        l = new yO;
        this.Fb = m = new GO(e, this.O, g,
            k, this.xa, E(this.Qk, this));
        var n = this.ja.D;
        l.A[1] = m;
        l.A[2] = new IO(e, g, k, n);
        this.Pe = new UO;
        this.B = new sK(this.ha, e, this.G, this.F, this.D, this.xa, this.L, a, this.Y, C, C, l, this.Wa, null);
        gP(this, d);
        Ic(Wh(this.A), 1, !0) && c && b.F && hP(this, b.F, d);
        Ic(Wh(this.A), 0, !0) && c && b.J && iP(this, b.J, d, b.da || void 0);
        Ic(Wh(this.A), 0, !0) && c && b.K && jP(this, b.K, d);
        Ic(Wh(this.A), 0, !0) && c && b.I && kP(this, b.I, d);
        Ic(Wh(this.A), 0, !0) && c && b.B && lP(this, b.B, d);
        this.P = null;
        (e = b.N) && mP(this, e, d);
        this.da = Gy(Fy());
        e = Fy();
        a = new nl(1E3);
        this.Oe =
            Gy(e, a);
        nP(this, c ? null : b.X, d);
        oP(this, c ? null : b.P, d);
        this.Ye = Gy(f);
        (f = b.aa) && pP(this, f, d);
        c && b.M && qP(this, b.M, d);
        this.Ta = null;
        c && b.L && rP(this, b.L, d);
        b = Fy();
        this.N = Gy(b);
        this.N.listen(this.Ck, this);
        this.Db = Gy(b);
        this.Db.listen(this.Ek, this);
        this.Ma = !1;
        this.U = Gy(Fy());
        this.U.listen(this.Dk, this);
        this.na = null;
        this.ga = [];
        this.Ga = new DM;
        Vk(w, "resize", E(this.ae, this, d), !1, this);
        sP(this, d);
        this.ae(d);
        d.done("main-actionflow-branch")
    }
    H(hN, il);
    q = hN.prototype;
    q.bind = function(a, b, c, d, e, f, g) {
        By(this.da, a, g);
        By(this.$, b, g);
        By(this.width, c, g);
        By(this.height, d, g);
        By(this.Xe, e, g);
        By(this.J, f, g)
    };

    function sP(a, b) {
        a.B.C(function(b, d) {
            a.C = b;
            By(a.C.width, a.width, d);
            By(a.C.height, a.height, d);
            By(a.C.aa, a.Pe.A, d);
            By(a.N, a.C.S, d);
            By(a.Db, a.C.H, d);
            By(a.U, a.C.B, d);
            for (b = 0; b < a.ga.length; ++b) UE(a.C.F.get(), a.ga[b].oa, a.ga[b].sa);
            a.ga.length = 0;
            a.ae(d)
        }, b)
    }

    function gP(a, b) {
        if (Ic(a.A, 9, !0)) {
            var c = new OO(O(Uh(a.A), 0), a.O);
            xj([c, a.B], function(b) {
                var d = c.A(),
                    f = a.B.A();
                d.bind(f.J, f.O, f.P, f.F, f.B, f.H, b)
            }, b);
            c.get(C, b)
        }
    }
    q.fa = function() {
        var a = this.C && this.C.S.get();
        a || (a = new le);
        return a
    };
    q.view = function(a, b) {
        if (K(a, 0) || K(a, 1)) {
            var c = new wo(this.L, "move_camera"),
                d = this;
            this.B.get(function(c, f) {
                d.C = c;
                tP(d, a, f, b)
            }, c);
            c.done("main-actionflow-branch")
        }
    };

    function tP(a, b, c, d) {
        if (a.Ma) EM(a.Ga, {
            view: b,
            oa: c
        });
        else {
            K(b, 0) && (a.Ma = !0);
            var e = function() {
                d && d(c);
                K(b, 0) && (a.Ma = !1);
                if (!a.Ga.Oa()) {
                    var e = FM(a.Ga);
                    tP(a, e.view, e.oa)
                }
            };
            if (!K(b, 0) && K(b, 1)) {
                var f = a.C.S.get();
                if (f) {
                    var g = new le;
                    U(g, f);
                    Cs(b.fa(), g);
                    VE(a.C.F.get(), g, a.U.get() || null, YO(L(b, 2, 2)), c, e)
                }
            } else XO(b.Da()) && (f = new RO, U(f, b), (g = new le(P(f, 1)), K(g, 2) && K(ue(g), 0)) || (Be(ve(g), a.width.get() || 1), De(ve(g), a.height.get() || 1)), uP(a, f, c, e))
        }
    }
    q.Lb = function(a) {
        var b = this.Fb.A;
        b ? b.Lb(a) : this.H.Lc = a;
        this.X && this.X.Lb(a)
    };
    q.Qk = function(a) {
        null === this.H.Lc || a.Lb(this.H.Lc);
        null !== this.H.zoom && (a.ga = this.H.zoom);
        a.Pb = Ic(Wh(this.A), 2);
        this.H = {
            Lc: null,
            zoom: null
        }
    };

    function uP(a, b, c, d) {
        var e = new le;
        U(e, b.fa());
        VE(a.C.F.get(), e, b.Da(), YO(L(b, 2, 2)), c, d)
    }
    q.ae = function(a) {
        var b = "ga" === this.aa.get() ? this.Pb : this.ha,
            c = b.clientWidth,
            b = b.clientHeight;
        if (this.ja.C) {
            var d = this.ja.C,
                e = YB(this.ja.getContext()),
                f;
            f = window;
            f = y(f.devicePixelRatio) ? f.devicePixelRatio : f.matchMedia ? dk(3) || dk(2) || dk(1.5) || dk(1) || .75 : 1;
            ZB(d, e, f, c, b)
        }
        this.width.set(c, a);
        this.height.set(b, a)
    };

    function mP(a, b, c) {
        a.P = new KO(a.G, a.F, "pano-floorpicker", a.D, b);
        xj([a.P, a.B], function(b) {
            var c = a.P.A(),
                d = a.B.A(),
                g = Hy(new Ey);
            c.bind(a.height, Gy(VD), d.F, d.B, d.S, b);
            c.kn(g, b)
        }, c)
    }

    function hP(a, b, c) {
        var d = new JO(b, a.F, a.G, a.I, new Ee(a.A.data[1]), a.D, O(new Sh(a.A.data[3]), 0), O(new Sh(a.A.data[3]), 1), !1);
        yj([d, a.B], function() {
            var b = a.B.A();
            d.A().bind(b.J, b.A.ga, b.B, c)
        }, c)
    }

    function jP(a, b, c) {
        var d = new aP(b, a.G, a.F, a.D, a.I);
        yj([d, a.B], function(b) {
            var c = d.A(),
                e = a.B.A();
            c.bind(e.S, e.da, e.B, a.Ua, b)
        }, c)
    }

    function nP(a, b, c) {
        b && (new MO(a.G, a.A, a.ja, a.I, a.Y, a.xa, a.aa, b)).get(function(b, c) {
            a.X = b;
            b.bind(a.da, a.Oe, a.$, a.width, a.height, a.J, a.N, a.Ye, c);
            b.Hi(a.H.zoom);
            b.Lb(a.H.Lc)
        }, c)
    }

    function oP(a, b, c) {
        b && (new LO(a.G, a.F, a.D, a.aa, a.ha, b)).get(function(b, c) {
            b.bind(a.da, a.$, a.U, a.J, a.N, c)
        }, c)
    }

    function pP(a, b, c) {
        var d = new SO(a.G, a.F, a.D, a.O, a.xa, O(Uh(a.A), 13), O(a.A, 13), a.I, b);
        d.C(function(b, c) {
            b.bind(a.J, a.da, a.$, c)
        }, c);
        var e = a.J.listen(function() {
            var b = a.J.get();
            if (!y(b)) throw Error("undefined in KVO getRequired");
            b && (d.get(C, c), e.Ub())
        })
    }

    function qP(a, b, c) {
        (new ZO(b, a.F, a.G, a.I, a.D)).get(function(b, c) {
            b.bind(a.da, a.$, c)
        }, c)
    }

    function rP(a, b, c) {
        a.Ta = new $O(b, a.G, a.A, a.F, a.I, a.D);
        yj([a.Ta, a.B], function(b) {
            var c = a.Ta.A(),
                d = a.B.A();
            c.bind(d.B, a.N, a.J, b)
        }, c)
    }

    function iP(a, b, c, d) {
        var e = new WO(b, a.G, a.F, function(b, d) {
            var e = d.Vb;
            a.I.get(function(a) {
                a.G(b, "_blank", e)
            }, c)
        }, a.D, a.O, d, void 0);
        xj([e, a.B], function(b) {
            var c = e.A(),
                d = a.B.A();
            c.bind(d.B, d.H, b)
        }, c);
        e.get(function(a, b) {
            a.wb(b)
        }, c)
    }

    function kP(a, b, c) {
        var d = new VO(b, a.G, a.F, a.D);
        yj([d, a.B], function(b) {
            var c = d.A(),
                e = a.B.A();
            c.bind(e.S, a.Ua, a.Xd, e.X, e.B, b)
        }, c)
    }

    function lP(a, b, c) {
        var d = new AO(b, a.G, a.F, a.D);
        yj([d, a.B], function(b) {
            var c = d.A(),
                e = a.B.A();
            c.bind(e.S, a.Ua, a.Xd, e.X, e.B, b)
        }, c)
    }
    q.Ck = function() {
        var a = this.N.get();
        a || (a = new le);
        this.dispatchEvent(new Ek("CameraChanged", a))
    };

    function vP(a, b) {
        var c = new wo(a.L, "show_road_labels");
        a.Zb = b;
        FO(a.Wa, function(b) {
            b.Y = a.Zb;
            Mx(b, c)
        }, c);
        c.done("main-actionflow-branch")
    }
    q.Ek = function() {
        var a = this.Db.get();
        a || (a = new le);
        this.dispatchEvent(new Ek("StableCameraChanged", a))
    };
    q.Dk = function(a) {
        var b = this.U.get();
        if (b) {
            var c = Is(Ys(b).ia());
            (O(b, 5) || c && S(c, 7)) && this.P && this.P.get(C, a);
            a = Ys(b);
            this.na && a && zh(this.na) == zh(a) && Ms(this.na, a) || (this.na = new wh(Pc(a)), this.dispatchEvent(new Ek("PhotoChanged", a)))
        }
    };

    function wP(a, b, c) {
        c = c || new wo(a.L, "wait_for_render");
        a.C ? UE(a.C.F.get(), c, b) : a.ga.push({
            oa: c,
            sa: c.sa(b, "viewer-wait-for-stable")
        })
    };

    function xP(a, b, c, d, e, f) {
        this.P = c;
        this.da = d;
        var g = document.createElement("div");
        this.I = g;
        a.appendChild(g);
        g.style.height = g.style.width = "100%";
        a = document.createElement("canvas");
        c = new bE(a);
        var k = d = null;
        if ("html5" != e && "html4" != e) try {
            if ("webgl" == e) d = pO(c);
            else if ("webgl_debug" == e) {
                var l = new gE;
                l.B = !0;
                l.A = !0;
                d = pO(c, l)
            }
        } catch (p) {}
        this.M = !!d;
        d || "html4" == e || (k = a.getContext("2d"));
        e = new yN(d, k, g);
        e = new eN(g, c, e);
        this.L = l = new Xh;
        l.A.data[14] = !0;
        b = new pN(b);
        c = {};
        for (k = 0; k < S(b, 8); ++k) c[Lc(b, 8, k)] = !0;
        d = new qN(b.data[24]);
        var k = O(d, 8),
            m = c[43] ? "maps_sv.tactile_lite" : "apiv3";
        l.A.data[15] = m;
        Ic(new oN(b.data[2]), 15) || (k = k.replace("http:", "https:"));
        Yh(l, k);
        for (var n = S(d, 7), k = 0; k < n; ++k) Zh(l, Lc(d, 7, k) + "?cb_client=" + m);
        m = S(d, 10);
        for (k = 0; k < m; ++k) $h(l, Lc(d, 10, k));
        ai(l, O(d, 4));
        bi(l, O(d, 6));
        (new Th(P(l.A, 4))).data[2] = !0;
        ci(l, O(new oN(b.data[2]), 0));
        di(l, O(new oN(b.data[2]), 1));
        l.A.data[9] = !1;
        ei(l, !!c[56]);
        this.A = gN(l, e);
        f && (b = new Do({}), b = new wo(b, "apiv3"), wP(this.A, function() {
            f()
        }, b));
        b = new wq;
        b.listen(this.A, "CameraChanged",
            E(this.$, this));
        b.listen(this.A, "PhotoChanged", E(this.aa, this));
        nb("touchstart touchmove touchend mousedown mousemove mouseup pointerdown pointermove pointerup MSPointerDown MSPointerMove MSPointerUp click".split(" "), function(a) {
            var b;
            g.addEventListener ? (g.addEventListener(a, rN, void 0), new vN(g, a, rN, 1)) : g.attachEvent ? (b = new vN(g, a, rN, 2), g.attachEvent("on" + a, xN(b))) : (g["on" + a] = rN, new vN(g, a, rN, 3))
        });
        this.D = this.K = "";
        this.G = {};
        this.C = new nN(0, 0, 0);
        this.F = !1;
        this.H = a.height / a.width;
        this.B = !1
    }
    xP.prototype.J = function() {
        if (!this.F) {
            var a = new RO,
                b = new le(P(a, 1)),
                c = !1,
                d = this.D;
            d && this.K != this.D && (this.G[d] && (a.data[2] = 3, re(b).data[2] = this.G[d].lat, re(b).data[1] = this.G[d].lng, ze(re(b), 3)), c = new Vs(P(a, 0)), c.data[0] = 1, c = Zs(c), "F:" == d.substring(0, 2) ? (c.data[1] = 4, c.data[0] = d.substring(2)) : (c.data[1] = 0, c.data[0] = d), c = !0, this.K = d);
            this.C && (b.data[3] = Kb(2 * Math.atan(Math.pow(2, 1 - this.C.zoom) * this.H)), b = te(b), b.data[0] = this.C.heading, b.data[1] = this.C.pitch + 90, c = !0);
            c && this.A.view(a)
        }
        this.B = !1
    };
    xP.prototype.$ = function(a) {
        this.B || (this.F = !0, a = a.target, this.C = new nN(Ae(se(a)), N(se(a), 1) - 90, 1 - Math.log(Math.tan(Jb(pe(a)) / 2) / this.H) / Math.log(2)), this.P(), this.F = !1)
    };
    xP.prototype.aa = function(a) {
        this.B || (this.F = !0, a = a.target, this.D = 4 == L(a, 1, 99) ? "F:" + a.za() : a.za(), this.da(), this.K = 4 == L(a, 1, 99) ? "F:" + a.za() : a.za(), this.F = !1)
    };
    xP.prototype.N = function(a, b) {
        var c = this.A,
            d = "ga" === c.aa.get() ? c.Pb : c.ha;
        Wn(d, a, b);
        d = new wo(c.L, "resize");
        c.ae(d);
        d.done("main-actionflow-branch");
        this.H = b / a
    };
    xP.prototype.setSize = xP.prototype.N;
    xP.prototype.Lb = function(a) {
        this.A.Lb(a)
    };
    xP.prototype.enableClickToGo = xP.prototype.Lb;
    xP.prototype.O = function(a) {
        vP(this.A, a)
    };
    xP.prototype.showRoadLabels = xP.prototype.O;
    xP.prototype.U = function(a) {
        var b = this.A,
            c = b.Fb.A;
        c ? c.ga = a : b.H.zoom = a;
        b.X && b.X.Hi(a)
    };
    xP.prototype.enableScrollToZoom = xP.prototype.U;
    xP.prototype.ga = function(a, b) {
        if (this.M && a && !b) {
            var c = document.createElement("canvas");
            b = c.getContext("2d");
            this.I.textContent = "";
            c = new bE(c);
            b = new yN(null, b, this.I);
            b = new eN(this.I, c, b);
            this.A = gN(this.L, b);
            this.M = !1;
            this.J()
        }
        Sr = {
            Pg: E(function(b) {
                if (!a) return null;
                if (b = a.Qh(b)) {
                    var c = b.tiles.worldSize.width,
                        d = b.tiles.worldSize.height,
                        g = b.tiles.tileSize.width,
                        k = b.tiles.tileSize.height,
                        l = new fh;
                    rh(l).data[0] = 2;
                    rh(l).data[1] = 2;
                    (new ld(P(rh(l), 2))).data[0] = d;
                    (new ld(P(rh(l), 2))).data[1] = c;
                    (new ld(P(new ef(P(rh(l),
                        3)), 1))).data[0] = k;
                    (new ld(P(new ef(P(rh(l), 3)), 1))).data[1] = g;
                    for (var m = [{
                            width: c,
                            height: d
                        }]; c > g || d > k;) c = c + 1 >> 1, d = d + 1 >> 1, m.push({
                        width: c,
                        height: d
                    });
                    for (c = m.length - 1; 0 <= c; --c) d = new ld(P(new ff(Mc(new ef(P(rh(l), 3)), 0)), 0)), d.data[1] = m[c].width, d.data[0] = m[c].height;
                    m = new Bg(Mc(l, 5));
                    (new Cg(P(m, 0))).data[0] = 1;
                    m = new Xe(P(new Te(P(m, 1)), 2));
                    m.data[0] = b.tiles.centerHeading;
                    m.data[1] = 90;
                    m.data[2] = 0;
                    b = l
                } else b = null;
                return b
            }, this),
            Tm: function(b, c, f, g) {
                return a.Qh(b).tiles.getTileUrl(b, c, f, g)
            }
        }
    };
    xP.prototype.registerPanoProvider = xP.prototype.ga;
    xP.prototype.Y = h("C");
    xP.prototype.getPov = xP.prototype.Y;
    xP.prototype.na = function(a) {
        this.C = a;
        this.B || (Kq(this.J, this), this.B = !0)
    };
    xP.prototype.setPov = xP.prototype.na;
    xP.prototype.X = h("D");
    xP.prototype.getPano = xP.prototype.X;
    xP.prototype.ja = function(a) {
        this.D = a;
        this.B || (Kq(this.J, this), this.B = !0)
    };
    xP.prototype.setPano = xP.prototype.ja;
    xP.prototype.ha = ba("G");
    xP.prototype.setNeighborLocation = xP.prototype.ha;
    Ga("google.maps.internal.iv", function(a, b, c, d, e, f) {
        return new xP(a, b, c, d, e, f)
    });
}).call(this);
google.maps.__gjsload__('imagery_viewer', function(_) {
    var ww = _.ga();
    ww.prototype.b = window.google.maps.internal && window.google.maps.internal.iv;
    delete window.google.maps.internal;
    _.rc("imagery_viewer", new ww);
});

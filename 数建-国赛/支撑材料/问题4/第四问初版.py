import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from scipy.stats import pearsonr
from statsmodels.nonparametric.smoothers_lowess import lowess
from sklearn.model_selection import StratifiedKFold
from sklearn.pipeline import Pipeline
from sklearn.impute import SimpleImputer
from sklearn.preprocessing import StandardScaler
from sklearn.neural_network import MLPClassifier
from sklearn.inspection import permutation_importance
from sklearn.metrics import (
    roc_curve, auc,
    precision_recall_curve, average_precision_score,
    confusion_matrix, accuracy_score, precision_score, recall_score, f1_score
)
plt.rcParams["font.family"] = ["Arial Unicode MS"]
plt.rcParams['axes.unicode_minus'] = False

FILE_PATH = r"/Users/ruanji/Desktop/数建/附件.xlsx"
SHEET_NAME = "女胎检测数据"
N_SPLITS = 5
TOPN_FEAT = 10
OUTDIR = "./"  

MLP_PARAMS = {
    "hidden_layer_sizes": (128, 64),
    "activation": "relu",
    "solver": "adam",
    "max_iter": 10000,
    "early_stopping": True,
    "validation_fraction": 0.2,
    "random_state": 42,
    "learning_rate_init": 0.0005
}

PLOT_CONFIG = {
    "heatmap_font": 6,
    "axis_label_font": 10,
    "title_font": 11,
    "annot_font": 8,
    "color_map": "coolwarm",
    "fold_colors": ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd"],
    "fold_grays": [0.1, 0.3, 0.5, 0.7, 0.9],
    "fold_linestyles": ["-", "--", "-.", ":", (0, (5, 10))],
    "fold_markers": ["o", "s", "^", "D", "v"],
    "sig_color": "red",
    "sig_gray": 0.2,
    "fit_line_color": "#d62728",
    "fit_line_gray": 0.1,
    "save_fig": True,
    "show_fig": True
}

plt.rcParams['font.sans-serif'] = ['SimHei', 'Arial']
plt.rcParams['axes.unicode_minus'] = False
plt.rcParams['figure.dpi'] = 100
plt.rcParams['savefig.dpi'] = 180
plt.rcParams['interactive'] = True

def week_to_float(w):
    if isinstance(w, str) and "w" in w:
        try:
            w0, d = w.split("w")
            d = d.replace("+", "")
            d = int(d) if d else 0
            return int(w0) + d/7.0
        except Exception:
            return np.nan
    try:
        return float(w)
    except Exception:
        return np.nan

def balance_sample(X, y):
    from sklearn.utils import resample
    X_major = X[y == 0]
    X_minor = X[y == 1]
    y_major = y[y == 0]
    y_minor = y[y == 1]
    
    X_minor_resampled, y_minor_resampled = resample(
        X_minor, y_minor,
        replace=True,
        n_samples=len(X_major),
        random_state=42
    )
    
    X_balanced = np.vstack((X_major, X_minor_resampled))
    y_balanced = np.hstack((y_major, y_minor_resampled))
    return X_balanced, y_balanced

def get_color_or_gray(is_color, color_val, gray_val):
    return color_val if is_color else str(gray_val)

df_raw = pd.read_excel(FILE_PATH, sheet_name=SHEET_NAME).copy()
df_raw["label"] = (~df_raw["染色体的非整倍体"].isna()).astype(int)
df_raw["孕周数"] = df_raw["检测孕周"].apply(week_to_float)

CAND_FEATURES = [
    "X染色体的Z值","13号染色体的Z值","18号染色体的Z值","21号染色体的Z值",
    "X染色体浓度","13号染色体浓度","18号染色体浓度","21号染色体浓度",
    "13号染色体的GC含量","18号染色体的GC含量","21号染色体的GC含量","GC含量",
    "原始读段数","唯一比对的读段数","在参考基因组上比对的比例",
    "重复读段的比例","被过滤掉读段数的比例",
    "孕妇BMI","年龄","检测抽血次数","孕周数"
]
FEATURES = [c for c in CAND_FEATURES if c in df_raw.columns]
use_cols = FEATURES + ["label"]
data = df_raw[use_cols].copy()

X = data[FEATURES].to_numpy()
y = data["label"].to_numpy().astype(int)
X_clean = []
y_clean = []
for i in range(len(X)):
    if np.all(np.abs((X[i] - np.nanmean(X, axis=0)) / np.nanstd(X, axis=0)) < 5):
        X_clean.append(X[i])
        y_clean.append(y[i])
X = np.array(X_clean)
y = np.array(y_clean)

print(f"数据规模（清洗后）：{X.shape[0]}个样本，{X.shape[1]}个特征")
print(f"正样本（非整倍体）：{y.sum()}个，负样本（正常）：{(1-y).sum()}个")

pipe = Pipeline([
    ("imp", SimpleImputer(strategy="median")),
    ("scaler", StandardScaler()),
    ("mlp", MLPClassifier(**MLP_PARAMS))
])

skf = StratifiedKFold(n_splits=N_SPLITS, shuffle=True, random_state=42)
oof_proba = np.zeros(len(X))
oof_pred50 = np.zeros(len(X), dtype=int)
fold_metrics = []
fold_roc_data = []

for fold, (tr_idx, te_idx) in enumerate(skf.split(X, y), 1):
    X_tr, X_te = X[tr_idx], X[te_idx]
    y_tr, y_te = y[tr_idx], y[te_idx]
    
    X_tr_balanced, y_tr_balanced = balance_sample(X_tr, y_tr)
    print(f"Fold{fold}：训练集原始{len(X_tr)}个样本 → 平衡后{len(X_tr_balanced)}个样本")
    
    pipe.fit(X_tr_balanced, y_tr_balanced)
    
    proba_te = pipe.predict_proba(X_te)[:, 1]
    pred50_te = (proba_te >= 0.5).astype(int)
    oof_proba[te_idx] = proba_te
    oof_pred50[te_idx] = pred50_te
    
    fpr, tpr, _ = roc_curve(y_te, proba_te)
    roc_auc = auc(fpr, tpr)
    fold_roc_data.append({"fold": fold, "fpr": fpr, "tpr": tpr, "auc": roc_auc})
    
    ap = average_precision_score(y_te, proba_te)
    acc = accuracy_score(y_te, pred50_te)
    prec = precision_score(y_te, pred50_te, zero_division=0)
    rec = recall_score(y_te, pred50_te, zero_division=0)
    f1 = f1_score(y_te, pred50_te, zero_division=0)
    fold_metrics.append({
        "Fold": fold, "AUC": roc_auc, "AP": ap,
        "ACC": acc, "Precision": prec, "Recall": rec, "F1": f1
    })

df_folds = pd.DataFrame(fold_metrics)
fpr_all, tpr_all, thr_all = roc_curve(y, oof_proba)
roc_auc_all = auc(fpr_all, tpr_all)
ap_all = average_precision_score(y, oof_proba)
acc_all = accuracy_score(y, oof_pred50)
prec_all = precision_score(y, oof_pred50, zero_division=0)
rec_all = recall_score(y, oof_pred50, zero_division=0)
f1_all = f1_score(y, oof_pred50, zero_division=0)

youden = tpr_all - fpr_all
best_thr_idx = np.argmax(youden) if len(youden) > 0 else 0
best_thr = thr_all[best_thr_idx] if len(thr_all) > 0 else 0.5
oof_pred_best = (oof_proba >= best_thr).astype(int)
cm_best = confusion_matrix(y, oof_pred_best)
TP = cm_best[1, 1]
FP = cm_best[0, 1]
TN = cm_best[0, 0]
FN = cm_best[1, 0]

X_balanced, y_balanced = balance_sample(X, y)
pipe.fit(X_balanced, y_balanced)

perm_result = permutation_importance(
    pipe, X, y, n_repeats=10, random_state=42, n_jobs=1
)
feat_importance = pd.DataFrame({
    "特征": FEATURES,
    "平均重要性": perm_result.importances_mean,
    "重要性标准差": perm_result.importances_std
}).sort_values("平均重要性", ascending=False).reset_index(drop=True)

imputer = SimpleImputer(strategy="median")
X_imp = imputer.fit_transform(X)

is_color = True
figs = []

fig1, axes = plt.subplots(2, 2, figsize=(16, 12))
figs.append(fig1)

ax1 = axes[0, 0]
corr_list = []
p_list = []
sig_marker = []
for feat in FEATURES:
    valid_mask = ~np.isnan(data[feat])
    if np.sum(valid_mask) < 2:
        corr_list.append(0)
        p_list.append(1.0)
    else:
        corr, p = pearsonr(data.loc[valid_mask, feat], data.loc[valid_mask, "label"])
        corr_list.append(corr)
        p_list.append(p)
    sig_marker.append("*" if p < 0.05 else "ns")

corr_df = pd.DataFrame({
    "特征": FEATURES, "相关系数": corr_list, "显著性": sig_marker
}).sort_values("相关系数", ascending=True)
bar_colors = [get_color_or_gray(is_color, PLOT_CONFIG["sig_color"], PLOT_CONFIG["sig_gray"]) 
              if s == "*" else get_color_or_gray(is_color, "#cccccc", 0.8) 
              for s in corr_df["显著性"]]

bars = ax1.barh(corr_df["特征"], corr_df["相关系数"], color=bar_colors, alpha=0.8)
for i, (_, row) in enumerate(corr_df.iterrows()):
    ax1.text(
        row["相关系数"] + 0.01 if row["相关系数"] > 0 else row["相关系数"] - 0.01,
        i, f"{row['相关系数']:.3f}", va="center", ha="left" if row["相关系数"] > 0 else "right",
        fontsize=PLOT_CONFIG["annot_font"]
    )

ax1.axvline(x=0, linestyle="--", color=get_color_or_gray(is_color, "black", 0.5), alpha=0.8)
ax1.set_xlabel("Pearson相关系数（与非整倍体标签）", fontsize=PLOT_CONFIG["axis_label_font"])
ax1.set_title(
    f"特征与非整倍体相关性（AUC={roc_auc_all:.3f}）\n（红色/深灰=P<0.05显著）",
    fontsize=PLOT_CONFIG["title_font"]
)
ax1.tick_params(axis="y", labelsize=PLOT_CONFIG["annot_font"])

ax2 = axes[0, 1]
feat_corr = pd.DataFrame(X_imp, columns=FEATURES).corr().round(2)
sns.heatmap(
    feat_corr, ax=ax2, cmap=PLOT_CONFIG["color_map"] if is_color else "gray",
    center=0, annot=True, annot_kws={"fontsize": PLOT_CONFIG["heatmap_font"]},
    square=True, linewidths=0.3, cbar_kws={"shrink": 0.8, "label": "相关系数"}
)
ax2.set_title("特征间相关性热力图（数值=相关系数）", fontsize=PLOT_CONFIG["title_font"])
ax2.tick_params(axis="x", rotation=45, labelsize=PLOT_CONFIG["heatmap_font"] + 1)
ax2.tick_params(axis="y", labelsize=PLOT_CONFIG["heatmap_font"] + 1)

ax3 = axes[1, 0]
top2_feats = feat_importance["特征"].head(2).tolist() if len(FEATURES)>=2 else FEATURES[:2]
feat1_idx = FEATURES.index(top2_feats[0]) if top2_feats[0] in FEATURES else 0
feat2_idx = FEATURES.index(top2_feats[1]) if top2_feats[1] in FEATURES else 1
feat1_val = X_imp[:, feat1_idx]
feat2_val = X_imp[:, feat2_idx]

for label in [0, 1]:
    mask = y == label
    if np.sum(mask) == 0:
        continue
    color = get_color_or_gray(is_color, "#1f77b4" if label==0 else "#d62728", 
                             PLOT_CONFIG["fold_grays"][label+2])
    marker = "o" if label==0 else "s"
    ax3.scatter(
        feat1_val[mask], feat2_val[mask],
        c=color, marker=marker, s=50, alpha=0.7,
        label=f"{'正常' if label==0 else '非整倍体'}（n={np.sum(mask)}）"
    )
    ax3.text(
        np.mean(feat1_val[mask]), np.mean(feat2_val[mask]),
        f"n={np.sum(mask)}", ha="center", va="center",
        fontsize=PLOT_CONFIG["annot_font"], fontweight="bold"
    )

ax3.set_xlabel(top2_feats[0], fontsize=PLOT_CONFIG["axis_label_font"])
ax3.set_ylabel(top2_feats[1], fontsize=PLOT_CONFIG["axis_label_font"])
ax3.set_title(f"Top2重要特征散点图", fontsize=PLOT_CONFIG["title_font"])
ax3.legend(fontsize=PLOT_CONFIG["annot_font"])
ax3.grid(alpha=0.3, linestyle="--")

ax4 = axes[1, 1]
sorted_idx = np.argsort(oof_proba)
sorted_proba = oof_proba[sorted_idx]
sorted_y = y[sorted_idx]
if len(sorted_proba) > 10:
    lowess_fit = lowess(sorted_y, sorted_proba, frac=0.3)
    ax4.plot(
        lowess_fit[:, 0], lowess_fit[:, 1],
        color=get_color_or_gray(is_color, PLOT_CONFIG["fit_line_color"], PLOT_CONFIG["fit_line_gray"]),
        linewidth=2, label="LOWESS拟合线"
    )

for label in [0, 1]:
    mask = sorted_y == label
    if np.sum(mask) == 0:
        continue
    color = get_color_or_gray(is_color, "#1f77b4" if label==0 else "#d62728", 
                             PLOT_CONFIG["fold_grays"][label+2])
    ax4.scatter(
        sorted_proba[mask], sorted_y[mask],
        c=color, s=30, alpha=0.5, label=f"{'正常' if label==0 else '非整倍体'}"
    )

residuals = oof_proba - y
ax4.text(
    0.05, 0.95,
    f"AUC={roc_auc_all:.3f}\n准确率={acc_all:.3f}\n召回率={rec_all:.3f}",
    transform=ax4.transAxes, ha="left", va="top",
    bbox=dict(boxstyle="round", facecolor="white", alpha=0.8),
    fontsize=PLOT_CONFIG["annot_font"]
)

ax4.set_xlabel("BP模型预测概率", fontsize=PLOT_CONFIG["axis_label_font"])
ax4.set_ylabel("真实标签（0=正常，1=非整倍体）", fontsize=PLOT_CONFIG["axis_label_font"])
ax4.set_title("预测概率与真实标签拟合曲线", fontsize=PLOT_CONFIG["title_font"])
ax4.legend(fontsize=PLOT_CONFIG["annot_font"])
ax4.grid(alpha=0.3, linestyle="--")

fig1_path = os.path.join("/Users/ruanji/Desktop/", "bp_correlation_scatter_fit.png")
plt.tight_layout()
plt.savefig(fig1_path, bbox_inches="tight")
print(f"图1已保存至：{fig1_path}")
if PLOT_CONFIG["show_fig"]:
    plt.show(block=False)

fig2, axes = plt.subplots(1, 2, figsize=(16, 6))
figs.append(fig2)

ax1 = axes[0]
for i, roc_data in enumerate(fold_roc_data):
    if i >= len(PLOT_CONFIG["fold_colors"]):
        break
    fold = roc_data["fold"]
    color = get_color_or_gray(is_color, PLOT_CONFIG["fold_colors"][i], PLOT_CONFIG["fold_grays"][i])
    linestyle = PLOT_CONFIG["fold_linestyles"][i]
    marker = PLOT_CONFIG["fold_markers"][i]
    ax1.plot(
        roc_data["fpr"], roc_data["tpr"],
        color=color, linestyle=linestyle, marker=marker, markersize=3, alpha=0.7,
        label=f"Fold{fold} (AUC={roc_data['auc']:.3f})"
    )

ax1.plot(
    fpr_all, tpr_all,
    color=get_color_or_gray(is_color, "#d62728", 0.1),
    linewidth=3, linestyle="-", label=f"总体OOF (AUC={roc_auc_all:.3f})"
)
ax1.plot(
    [0, 1], [0, 1],
    color=get_color_or_gray(is_color, "gray", 0.5),
    linestyle="--", alpha=0.8, label="随机猜测"
)

auc_mean = df_folds["AUC"].mean()
auc_std = df_folds["AUC"].std()
ax1.text(
    0.6, 0.1,
    f"AUC均值±标准差：{auc_mean:.3f}±{auc_std:.3f}\n波动系数：{auc_std/auc_mean:.3f}",
    transform=ax1.transAxes, ha="left", va="bottom",
    bbox=dict(boxstyle="round", facecolor="white", alpha=0.8),
    fontsize=PLOT_CONFIG["annot_font"]
)

ax1.set_xlabel("假阳性率（FPR）", fontsize=PLOT_CONFIG["axis_label_font"])
ax1.set_ylabel("真阳性率（TPR）", fontsize=PLOT_CONFIG["axis_label_font"])
ax1.set_title("5折ROC曲线叠加（波动小=稳健）", fontsize=PLOT_CONFIG["title_font"])
ax1.legend(fontsize=PLOT_CONFIG["annot_font"], loc="lower right")
ax1.grid(alpha=0.3, linestyle="--")

ax2 = axes[1]
residuals = oof_proba - y
n, bins, patches = ax2.hist(
    residuals, bins=20, alpha=0.7,
    color=get_color_or_gray(is_color, "#2ca02c", 0.4),
    edgecolor=get_color_or_gray(is_color, "black", 0.2), linewidth=0.5
)
ax2.axvline(
    x=0, color=get_color_or_gray(is_color, "red", 0.1),
    linestyle="--", linewidth=2, label="残差=0（理想值）"
)

res_mean = np.mean(residuals)
res_std = np.std(residuals)
ax2.text(
    0.95, 0.95,
    f"残差均值：{res_mean:.3f}\n残差标准差：{res_std:.3f}\n样本数：{len(residuals)}",
    transform=ax2.transAxes, ha="right", va="top",
    bbox=dict(boxstyle="round", facecolor="white", alpha=0.8),
    fontsize=PLOT_CONFIG["annot_font"]
)

ax2.set_xlabel("残差（预测概率 - 真实标签）", fontsize=PLOT_CONFIG["axis_label_font"])
ax2.set_ylabel("样本数量", fontsize=PLOT_CONFIG["axis_label_font"])
ax2.set_title("预测残差分布（集中在0=精度高）", fontsize=PLOT_CONFIG["title_font"])
ax2.legend(fontsize=PLOT_CONFIG["annot_font"])
ax2.grid(alpha=0.3, linestyle="--")

fig2_path = os.path.join("/Users/ruanji/Desktop/", "bp_robustness_residual.png")
plt.tight_layout()
plt.savefig(fig2_path, bbox_inches="tight")
print(f"图2已保存至：{fig2_path}")
if PLOT_CONFIG["show_fig"]:
    plt.show(block=False)

fig3, ax = plt.subplots(1, 1, figsize=(10, 6))
figs.append(fig3)

top_feats = feat_importance.head(TOPN_FEAT)
x_pos = np.arange(len(top_feats))
bars = ax.bar(
    x_pos, top_feats["平均重要性"],
    yerr=top_feats["重要性标准差"], capsize=5,
    color=[get_color_or_gray(is_color, PLOT_CONFIG["fold_colors"][i%5], PLOT_CONFIG["fold_grays"][i%5]) 
           for i in range(len(top_feats))],
    alpha=0.8, edgecolor=get_color_or_gray(is_color, "black", 0.2), linewidth=0.5
)

for i, (_, row) in enumerate(top_feats.iterrows()):
    ax.text(
        i, row["平均重要性"] + row["重要性标准差"] + 0.005,
        f"{row['平均重要性']:.3f}", ha="center", va="bottom",
        fontsize=PLOT_CONFIG["annot_font"]
    )

ax.set_xticks(x_pos)
ax.set_xticklabels(top_feats["特征"], rotation=45, ha="right", fontsize=PLOT_CONFIG["annot_font"])
ax.set_xlabel("特征", fontsize=PLOT_CONFIG["axis_label_font"])
ax.set_ylabel("置换重要性（AUC下降幅度）", fontsize=PLOT_CONFIG["axis_label_font"])
ax.set_title(f"Top{TOPN_FEAT}特征重要性（误差线=10次重复）", fontsize=PLOT_CONFIG["title_font"])
ax.grid(axis="y", alpha=0.3, linestyle="--")

fig3_path = os.path.join("/Users/ruanji/Desktop/", "bp_feature_importance.png")
plt.tight_layout()
plt.savefig(fig3_path, bbox_inches="tight")
print(f"图3已保存至：{fig3_path}")
if PLOT_CONFIG["show_fig"]:
    plt.show(block=True)

print("\n" + "="*60)
print("=== 总体预测精度指标（修正后）===")
print("="*60)
overall_df = pd.DataFrame({
    "指标": ["AUC-ROC", "AP-PR", "准确率(ACC)", "精确率(Precision)", "召回率(Recall)", "F1分数",
            "最优阈值(Youden)", "真阳性(TP)", "假阳性(FP)", "真阴性(TN)", "假阴性(FN)"],
    "数值": [f"{roc_auc_all:.4f}", f"{ap_all:.4f}", f"{acc_all:.4f}", f"{prec_all:.4f}", 
            f"{rec_all:.4f}", f"{f1_all:.4f}", f"{best_thr:.4f}",
            str(TP), str(FP), str(TN), str(FN)]
})
print(overall_df.to_string(index=False))

print("\n" + "="*60)
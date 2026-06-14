import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from sklearn.preprocessing import StandardScaler
from sklearn.tree import DecisionTreeRegressor
from scipy.stats import norm, t
from scipy.signal import savgol_filter
plt.rcParams["font.family"] = ["Arial Unicode MS"]
plt.rcParams['axes.unicode_minus'] = False

def week_to_float(w):
    if isinstance(w, str) and "w" in w:
        try:
            w0, d = w.split("w")
            d = d.replace("+","")
            d = int(d) if d else 0
            return int(w0) + d/7.0
        except:
            return np.nan
    try:
        return float(w)
    except:
        return np.nan

def logit(p):
    p = np.clip(p, 1e-6, 1-1e-6)
    return np.log(p/(1-p))

def build_design(w, b, extra_std=None):
    base = np.column_stack([np.ones_like(w), w, b, w**2, b**2, w*b])
    if extra_std is None:
        return base
    if extra_std.ndim == 1:
        extra_std = extra_std.reshape(1,-1)
    if extra_std.shape[0] == 1:
        extra_std = np.repeat(extra_std, repeats=len(w), axis=0)
    return np.column_stack([base, extra_std])

file_path = r"/Users/ruanji/Desktop/数建/附件.xlsx"
raw = pd.read_excel(file_path, sheet_name="男胎检测数据")
raw["BMI"] = raw["体重"] / ((raw["身高"]/100.0)**2)
raw["孕周数"] = raw["检测孕周"].apply(week_to_float)

extra_cols_all = [
    "年龄","身高","体重",
    "13号染色体的GC含量","18号染色体的GC含量","21号染色体的GC含量",
    "被过滤掉读段数的比例","原始测序数据的总读取数（个）","原始测序数据的总读段数（个）",
    "总读取中属于参考基因组上的读段比例（%）","检测抽血次数"
]

cols = ["孕周数","BMI","Y染色体浓度"] + [c for c in extra_cols_all if c in raw.columns]
df = raw[cols].dropna().copy()
df["Y染色体浓度"] = df["Y染色体浓度"].clip(1e-6, 1-1e-6)

extra_cols = [c for c in extra_cols_all if c in df.columns]
if len(extra_cols)>0:
    scaler = StandardScaler()
    extra_std = scaler.fit_transform(df[extra_cols].to_numpy())
else:
    scaler = None
    extra_std = None

z = logit(df["Y染色体浓度"].to_numpy())
X = build_design(df["孕周数"].to_numpy(), df["BMI"].to_numpy(), extra_std)
XtX = X.T @ X
XtX_inv = np.linalg.inv(XtX)
beta = XtX_inv @ X.T @ z
resid = z - X @ beta
n, p = X.shape
sigma2 = (resid @ resid) / (n-p)
sigma = np.sqrt(sigma2)
z_thr = logit(0.04)

def prob_reach(w, b, extra_std_block=None, sigma_scale=1.0):
    Xp = build_design(np.array(w), np.array(b), extra_std_block)
    zhat = Xp @ beta
    return 1 - norm.cdf((z_thr - zhat)/(sigma*sigma_scale))

q = 0.90
b_grid = np.linspace(np.percentile(df["BMI"],5), np.percentile(df["BMI"],95), 220)
w_grid = np.linspace(df["孕周数"].min(), df["孕周数"].max(), 360)

if len(extra_cols)>0:
    med_extra = np.median(df[extra_cols].to_numpy(), axis=0, keepdims=True)
    med_extra_std = scaler.transform(med_extra)
else:
    med_extra_std = None

def earliest_week_given_b(bv, q=0.90, sigma_scale=1.0):
    ws = w_grid
    bs = np.full_like(ws, bv)
    p = prob_reach(ws, bs, med_extra_std, sigma_scale=sigma_scale)
    ok = ws[p >= q]
    return ok[0] if ok.size>0 else np.nan

w_star = np.array([earliest_week_given_b(bv, q) for bv in b_grid])

mask = ~np.isnan(w_star)
tree = DecisionTreeRegressor(max_leaf_nodes=5, random_state=0)
tree.fit(b_grid[mask].reshape(-1,1), w_star[mask])
def extract_thresholds(dt):
    t = dt.tree_
    thr = []
    def dfs(node):
        if t.feature[node] != -2:
            thr.append(t.threshold[node])
            dfs(t.children_left[node]); dfs(t.children_right[node])
    dfs(0); return sorted(thr)
bounds = [float(b_grid[mask].min())] + extract_thresholds(tree) + [float(b_grid[mask].max())]
bounds = [round(x,2) for x in bounds]

XtX = X.T @ X
XtX_inv = np.linalg.inv(XtX)
se = np.sqrt(np.diag(sigma2 * XtX_inv))
tstat = beta / se
pval = 2*(1 - t.cdf(np.abs(tstat), df=n-p))
coef_names = ["常数","孕周","BMI","孕周^2","BMI^2","孕周×BMI"] + [f"标准化:{c}" for c in extra_cols]
coef_df = pd.DataFrame({
    "变量": coef_names,
    "系数": beta,
    "标准误": se,
    "t值": tstat,
    "p值": pval,
    "下限95%CI": beta - 1.96*se,
    "上限95%CI": beta + 1.96*se
})

R2 = 1 - (resid@resid)/(((z-z.mean())**2).sum())
AIC = n * np.log(sigma2) + 2 * p
BIC = n * np.log(sigma2) + p * np.log(n)
metrics = pd.DataFrame({
    "样本量n":[n], "参数数p":[p],
    "R2": 1 - (resid@resid)/(((z-z.mean())**2).sum()),
    "Adj-R2":[1 - (1-R2)*(n-1)/(n-p)],
    "sigma(logit域)":[sigma],
    "AIC":[AIC], "BIC":[BIC],
    "孕周覆盖":[f"{df['孕周数'].min():.2f}-{df['孕周数'].max():.2f}"],
    "BMI覆盖":[f"{df['BMI'].min():.2f}-{df['BMI'].max():.2f}"]
})

print("问题3_模型系数显著性：\n", coef_df.round(4))
print("\n问题3_拟合与信息准则指标：\n", metrics)

group_ext = []
for i in range(len(bounds)-1):
    lo, hi = bounds[i], bounds[i+1]
    in_group = (df["BMI"]>=lo) & (df["BMI"]<(hi if i<len(bounds)-2 else hi+1e-9))
    sub = df.loc[in_group].copy()
    if len(sub)==0:
        continue
    ws = w_grid
    mean_probs = []
    for wv in ws:
        pr = prob_reach(np.full(len(sub), wv),
                        sub["BMI"].to_numpy(),
                        scaler.transform(sub[extra_cols].to_numpy()) if len(extra_cols)>0 else None)
        mean_probs.append(pr.mean())
    mean_probs = np.array(mean_probs)
    ok = ws[mean_probs>=q]
    w_rec = ok[0] if ok.size>0 else np.nan
    group_ext.append({
        "组次":i+1,
        "BMI区间":f"[{lo},{hi}]",
        "样本量":len(sub),
        "推荐孕周":round(w_rec,2) if not np.isnan(w_rec) else None,
        "推荐孕周的平均达标比例(%)":round(mean_probs[ws==w_rec][0]*100,2) if not np.isnan(w_rec) else None
    })

group_df = pd.DataFrame(group_ext)
print("\n问题3_BMI分组增强指标：\n", group_df)

from mpl_toolkits.mplot3d import Axes3D
from matplotlib.colors import ListedColormap

plt.figure(figsize=(12, 10))

pred_prob = prob_reach(df["孕周数"].to_numpy(), df["BMI"].to_numpy(), 
                      scaler.transform(df[extra_cols].to_numpy()) if len(extra_cols)>0 else None)
actual = (df["Y染色体浓度"] >= 0.04).astype(int)

plt.subplot(2, 1, 1)
plt.scatter(df["孕周数"], df["BMI"], c=pred_prob, cmap='coolwarm', alpha=0.7, 
           vmin=0, vmax=1, s=50)
plt.colorbar(label='预测概率')
plt.scatter(df["孕周数"][actual==1], df["BMI"][actual==1], 
           edgecolors='k', facecolors='none', s=60, label='实际达标')
plt.title("实际达标 vs 预测概率")
plt.xlabel("孕周数（周）")
plt.ylabel("BMI")
plt.legend()

plt.subplot(2, 1, 2)
residuals = actual - pred_prob
plt.scatter(pred_prob, residuals, c=df["孕周数"], cmap='viridis', alpha=0.6)
plt.colorbar(label='孕周数')
plt.axhline(y=0, color='r', linestyle='--')
plt.title("预测残差分析")
plt.xlabel("预测概率")
plt.ylabel("残差 (实际 - 预测)")
plt.tight_layout()
plt.show()

fig = plt.figure(figsize=(12, 9))
ax = fig.add_subplot(111, projection='3d')

W, B = np.meshgrid(
    np.linspace(df["孕周数"].min(), df["孕周数"].max(), 80),
    np.linspace(np.percentile(df["BMI"],5), np.percentile(df["BMI"],95), 80)
)
P = prob_reach(W.ravel(), B.ravel(), med_extra_std).reshape(W.shape)

surf = ax.plot_surface(W, B, P, cmap='viridis', edgecolor='none', alpha=0.9)
fig.colorbar(surf, ax=ax, shrink=0.5, aspect=5, label='P(Y≥4%)')

for bound in bounds:
    ax.plot(np.linspace(W.min(), W.max(), 50), 
           np.ones(50)*bound, 
           np.ones(50)*0.5,
           'r-', linewidth=1.5, alpha=0.7)

xx, yy = np.meshgrid(np.linspace(W.min(), W.max(), 10), 
                    np.linspace(B.min(), B.max(), 10))
zz = np.ones(xx.shape) * q
ax.plot_surface(xx, yy, zz, color='red', alpha=0.3)

ax.set_title("3D 达标概率曲面 (含分组边界)")
ax.set_xlabel("孕周数（周）")
ax.set_ylabel("BMI")
ax.set_zlabel("P(Y≥4%)")
plt.tight_layout()
plt.show()

plt.figure(figsize=(10, 8))
ax = plt.subplot(111, polar=True)

metrics = []
for i in range(len(bounds)-1):
    lo, hi = bounds[i], bounds[i+1]
    in_group = (df["BMI"]>=lo) & (df["BMI"]<hi)
    if in_group.sum() == 0:
        continue
    
    group_pred = pred_prob[in_group]
    group_actual = actual[in_group]
    
    accuracy = np.mean((group_pred >= q) == (group_actual == 1))
    precision = np.sum((group_pred >= q) & (group_actual == 1)) / max(1, np.sum(group_pred >= q))
    recall = np.sum((group_pred >= q) & (group_actual == 1)) / max(1, np.sum(group_actual == 1))
    f1 = 2 * (precision * recall) / (precision + recall) if (precision + recall) > 0 else 0
    
    metrics.append({
        "group": f"组{i+1}",
        "accuracy": accuracy,
        "precision": precision,
        "recall": recall,
        "f1": f1
    })

categories = ['准确率', '精确率', '召回率', 'F1分数']
N = len(categories)
angles = [n / float(N) * 2 * np.pi for n in range(N)]
angles += angles[:1]

for i, metric in enumerate(metrics):
    values = [
        metric['accuracy'],
        metric['precision'],
        metric['recall'],
        metric['f1']
    ]
    values += values[:1]
    
    ax.plot(angles, values, linewidth=2, linestyle='solid', 
            label=f"{metric['group']} ({bounds[i]}-{bounds[i+1]})")
    ax.fill(angles, values, alpha=0.1)

ax.set_xticks(angles[:-1])
ax.set_xticklabels(categories)
ax.set_title("不同BMI组模型性能指标", y=1.1)
ax.legend(loc='upper right', bbox_to_anchor=(1.3, 1.1))
plt.tight_layout()
plt.show()
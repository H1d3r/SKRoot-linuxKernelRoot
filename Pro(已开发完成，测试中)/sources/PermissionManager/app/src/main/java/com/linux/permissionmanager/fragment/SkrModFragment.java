package com.linux.permissionmanager.fragment;

import android.app.Activity;
import android.content.pm.PackageInfo;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.widget.PopupMenu;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.linux.permissionmanager.R;
import com.linux.permissionmanager.adapter.SkrModAdapter;
import com.linux.permissionmanager.adapter.SkrModAdapter;
import com.linux.permissionmanager.adapter.SkrModPrinter;
import com.linux.permissionmanager.bridge.NativeBridge;
import com.linux.permissionmanager.model.SkrModItem;
import com.linux.permissionmanager.model.SkrModItem;
import com.linux.permissionmanager.utils.DialogUtils;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.File;
import java.net.URLDecoder;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

public class SkrModFragment extends Fragment {
    private Activity mActivity;
    private String mRootKey = "";

    private TextView mTextEmptyTips;
    private RecyclerView mSkrModRecyclerView;

    public SkrModFragment(Activity activity) {
        mActivity = activity;
    }
    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater,
                             @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_skr_mod, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mTextEmptyTips = view.findViewById(R.id.empty_tips_tv);
        mSkrModRecyclerView = view.findViewById(R.id.skr_mod_recycler_view);
        setupSkrModRecyclerView();
    }

    public void setRootKey(String rootKey) {
        mRootKey = rootKey;
    }

    public void setupSkrModRecyclerView() {
        String jsonAll = NativeBridge.getSkrootModuleList(mRootKey, false);
        List<SkrModItem> listAll = parseSkrModList(jsonAll);
        if(!listAll.isEmpty()) {
            String jsonRunning = NativeBridge.getSkrootModuleList(mRootKey, true);
            List<SkrModItem> listRunning = parseSkrModList(jsonRunning);

            Set<String> runningUuids = (listRunning == null) ?
                    Collections.emptySet() :
                    listRunning.stream()
                            .map(SkrModItem::getUuid)
                            .filter(Objects::nonNull)
                            .collect(Collectors.toSet());
            listAll.forEach(it -> it.setRunning(
                    it.getUuid() != null && runningUuids.contains(it.getUuid())
            ));
        }

        SkrModAdapter adapter = new SkrModAdapter(listAll, new SkrModAdapter.OnItemClickListener() {
            @Override
            public void onOpenWebUIBtnClick(View v, SkrModItem skrmod) {
                onOpenSkrModWebUI(skrmod);
            }
            @Override
            public void onMoreBtnClick(View v, SkrModItem skrmod) {
                showSkrModItemPopupMenu(v, skrmod);
            }
        });
        mSkrModRecyclerView.setLayoutManager(new LinearLayoutManager(getContext()));
        mSkrModRecyclerView.setAdapter(adapter);
        mTextEmptyTips.setVisibility(listAll.size() == 0 ? View.VISIBLE : View.GONE);
        mSkrModRecyclerView.setVisibility(listAll.size() == 0 ? View.GONE : View.VISIBLE);
    }

    private List<SkrModItem> parseSkrModList(String jsonStr) {
        List<SkrModItem> list = new ArrayList<>();
        try {
            JSONArray jsonArray = new JSONArray(jsonStr);
            for (int i = 0; i < jsonArray.length(); i++) {
                JSONObject jsonObject = jsonArray.getJSONObject(i);

                String name = URLDecoder.decode(jsonObject.getString("name"), "UTF-8");
                String ver = URLDecoder.decode(jsonObject.getString("ver"), "UTF-8");
                String desc = URLDecoder.decode(jsonObject.getString("desc"), "UTF-8");
                String author = URLDecoder.decode(jsonObject.getString("author"), "UTF-8");
                String uuid = URLDecoder.decode(jsonObject.getString("uuid"), "UTF-8");
                boolean web_ui = jsonObject.getBoolean("web_ui");
                String min_sdk_ver = URLDecoder.decode(jsonObject.getString("min_sdk_ver"), "UTF-8");

                SkrModItem e = new SkrModItem(name, desc, ver, uuid, author, min_sdk_ver, web_ui, false);
                list.add(e);
            }
        } catch (Exception e) {
            DialogUtils.showMsgDlg(mActivity, "发生错误", jsonStr, null);
            e.printStackTrace();
        }
        return list;
    }

    private void showSkrModItemPopupMenu(View v, SkrModItem skrMod) {
        PopupMenu popupMenu = new PopupMenu(requireContext(), v);
        popupMenu.getMenuInflater().inflate(R.menu.popup_skr_mod_item_menu, popupMenu.getMenu());
        popupMenu.setOnMenuItemClickListener(item -> {
            int itemId = item.getItemId();
            if (itemId == R.id.del) {
                onDeleteSkrMod(skrMod);
            } else if (itemId == R.id.details) {
                onShowDetailsSkrMod(skrMod);
            }
            return true;
        });
        popupMenu.show();
    }

    public void onAddSkrMod(String zipFilePath) {
        String tip = NativeBridge.installSkrootModule(mRootKey, zipFilePath);
        DialogUtils.showMsgDlg(mActivity, "执行结果", tip, null);
        setupSkrModRecyclerView();
    }

    private void onDeleteSkrMod(SkrModItem skrMod) {
        DialogUtils.showCustomDialog(
                mActivity,
                "确认",
                "确定要删除 " + skrMod.getName() + " 模块吗？",
                null,
                "确定", (dialog, which) -> {
                    dialog.dismiss();
                    String tip = NativeBridge.uninstallSkrootModule(mRootKey, skrMod.getUuid());
                    DialogUtils.showMsgDlg(mActivity, "执行结果", tip, null);
                    setupSkrModRecyclerView();
                },
                "取消", (dialog, which) -> {
                    dialog.dismiss();
                }
        );
    }

    private void onShowDetailsSkrMod(SkrModItem skrMod) {
        String details = SkrModPrinter.buildModuleMeta(skrMod);
        DialogUtils.showLogDialog(mActivity, details);
    }
    
    private void onOpenSkrModWebUI(SkrModItem skrMod) {
        String tip = NativeBridge.openSkrootModuleWebUI(mRootKey, skrMod.getUuid());
        DialogUtils.showMsgDlg(mActivity, "执行结果", tip, null);
    }

}

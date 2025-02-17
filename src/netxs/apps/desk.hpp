// Copyright (c) NetXS Group.
// Licensed under the MIT license.

#pragma once

namespace netxs::events::userland
{
    struct desk
    {
        EVENTPACK( desk, netxs::events::userland::root::custom )
        {
            GROUP_XS( ui, text ),

            SUBSET_XS( ui )
            {
                EVENT_XS( selected, text ),
            };
        };
    };
}

// desk: Sidebar menu.
namespace netxs::app::desk
{
    static constexpr auto id = "desk";
    static constexpr auto desc = "Taskbar menu";

    using events = ::netxs::events::userland::desk;

    namespace
    {
        auto app_template = [](auto& data_src, auto const& utf8)
        {
            auto danger_color    = skin::globals().danger;
            auto highlight_color = skin::globals().highlight;
            auto c4 = cell{}.bgc(highlight_color.bgc());
            auto x4 = cell{ c4 }.bga(0x00);
            auto c5 = danger_color;
            auto x5 = cell{ c5 }.alpha(0x00);
            auto fastfader = skin::globals().fader_fast;
            auto fader = skin::globals().fader_time;
            auto item_area = ui::pads::ctor(dent{ 1,0,1,0 }, dent{ 0,0,0,1 })
                ->plugin<pro::fader>(x4, c4, fastfader)
                ->plugin<pro::notes>(" Running instance:                          \n"
                                     "   Left click to go to running instance     \n"
                                     "   Right click to pull the running instance ")
                ->invoke([&](auto& boss)
                {
                    auto data_src_shadow = ptr::shadow(data_src);
                    auto boss_ptr_shadow = ptr::shadow(boss.This());
                    boss.LISTEN(tier::release, hids::events::mouse::button::click::left, gear, -, (data_src_shadow))
                    {
                        if (auto data_src = data_src_shadow.lock())
                        {
                            auto& inst = *data_src;
                            inst.SIGNAL(tier::preview, e2::form::layout::expose, inst);
                            auto& area = inst.base::area();
                            auto center = area.coor + (area.size / 2);
                            gear.owner.SIGNAL(tier::release, e2::form::layout::shift, center);  // Goto to the window.
                            gear.pass_kb_focus(inst);
                            gear.dismiss();
                        }
                    };
                    boss.LISTEN(tier::release, hids::events::mouse::button::click::right, gear, -, (data_src_shadow))
                    {
                        if (auto data_src = data_src_shadow.lock())
                        {
                            auto& inst = *data_src;
                            inst.SIGNAL(tier::preview, e2::form::layout::expose, inst);
                            auto viewport = e2::form::prop::viewport.param();
                            boss.SIGNAL(tier::anycast, e2::form::prop::viewport, viewport);
                            auto center = gear.area().coor + viewport.coor + (viewport.size / 2);
                            inst.SIGNAL(tier::preview, e2::form::layout::appear, center); // Pull window.
                            gear.pass_kb_focus(inst);
                            gear.dismiss();
                        }
                    };
                    boss.LISTEN(tier::release, e2::form::state::mouse, hits, -, (data_src_shadow))
                    {
                        if (auto data_src = data_src_shadow.lock())
                        {
                            data_src->SIGNAL(tier::release, e2::form::highlight::any, !!hits);
                        }
                    };
                });
            auto label_area = item_area->attach(ui::fork::ctor());
            auto mark_app = label_area->attach(slot::_1, ui::fork::ctor());
            auto mark = mark_app->attach(slot::_1, ui::pads::ctor(dent{ 2,1,0,0 }, dent{ 0,0,0,0 }))
                ->attach(ui::item::ctor(ansi::fgx(0xFF00ff00).add("‣"), faux));
            auto app_label = mark_app->attach(slot::_2, ui::item::ctor(ansi::fgc(whitelt).add(utf8).mgl(0).wrp(wrap::off).jet(bias::left), true, true));
            auto app_close_area = label_area->attach(slot::_2, ui::pads::ctor(dent{ 0,0,0,0 }, dent{ 0,0,1,1 }))
                ->template plugin<pro::fader>(x5, c5, fader)
                ->template plugin<pro::notes>(" Close instance ")
                ->invoke([&](auto& boss)
                {
                    auto data_src_shadow = ptr::shadow(data_src);
                    boss.LISTEN(tier::release, hids::events::mouse::button::click::left, gear, -, (data_src_shadow))
                    {
                        if (auto data_src = data_src_shadow.lock())
                        {
                            data_src->SIGNAL(tier::anycast, e2::form::quit, data_src);
                            gear.dismiss();
                        }
                    };
                });
            auto app_close = app_close_area->attach(ui::item::ctor("  ×  ", faux));
            return item_area;
        };
        auto apps_template = [](auto& data_src, auto& apps_map_ptr)
        {
            auto highlight_color = skin::globals().highlight;
            auto inactive_color  = skin::globals().inactive;
            auto c3 = highlight_color;
            auto x3 = cell{ c3 }.alpha(0x00);
            auto cA = inactive_color;

            auto apps = ui::list::ctor()
                ->invoke([&](auto& boss)
                {
                    boss.LISTEN(tier::release, e2::form::upon::vtree::attached, parent)
                    {
                        auto current_default = e2::data::changed.param();
                        boss.RISEUP(tier::request, e2::data::changed, current_default);
                        boss.SIGNAL(tier::anycast, events::ui::selected, current_default);
                    };
                });

            auto def_note = text{" Menu item:                           \n"
                                 "   Left click to start a new instance \n"
                                 "   Right click to set default app     "};
            auto conf_list_ptr = vtm::events::list::menu.param();
            data_src->RISEUP(tier::request, vtm::events::list::menu, conf_list_ptr);
            if (!conf_list_ptr || !apps_map_ptr) return apps;
            auto& conf_list = *conf_list_ptr;
            auto& apps_map = *apps_map_ptr;
            for (auto const& [class_id, stat_inst_ptr_list] : apps_map)
            {
                auto& [state, inst_ptr_list] = stat_inst_ptr_list;
                auto inst_id = class_id;
                auto& conf = conf_list[class_id];
                auto& obj_desc = conf.label;
                auto& obj_note = conf.notes;
                if (conf.splitter)
                {
                    auto item_area = apps->attach(ui::pads::ctor(dent{ 0,0,0,1 }, dent{ 0,0,1,0 }))
                        ->attach(ui::item::ctor(obj_desc, true, faux, true))
                        ->colors(cA.fgc(), cA.bgc())
                        ->template plugin<pro::notes>(obj_note);
                    continue;
                }
                auto item_area = apps->attach(ui::pads::ctor(dent{ 0,0,0,1 }, dent{ 0,0,1,0 }))
                    ->template plugin<pro::fader>(x3, c3, skin::globals().fader_fast)
                    ->template plugin<pro::notes>(obj_note.empty() ? def_note : obj_note)
                    ->invoke([&](auto& boss)
                    {
                        boss.mouse.take_all_events(faux);
                        //auto boss_shadow = ptr::shadow(boss.This());
                        //auto data_src_shadow = ptr::shadow(data_src);
                        boss.LISTEN(tier::release, hids::events::mouse::button::click::right, gear, -, (inst_id))
                        {
                            boss.SIGNAL(tier::anycast, events::ui::selected, inst_id);
                        };
                        boss.LISTEN(tier::release, hids::events::mouse::button::click::left, gear, -, (inst_id))
                        {
                            boss.SIGNAL(tier::anycast, events::ui::selected, inst_id);
                            static auto offset = dot_00;
                            auto viewport = e2::form::prop::viewport.param();
                            boss.SIGNAL(tier::anycast, e2::form::prop::viewport, viewport);
                            viewport.coor += gear.area().coor;;
                            offset = (offset + dot_21 * 2) % (viewport.size * 7 / 32);
                            gear.slot.coor = viewport.coor + offset + viewport.size * 1 / 32;
                            gear.slot.size = viewport.size * 3 / 4;
                            gear.slot_forced = faux;
                            boss.RISEUP(tier::request, e2::form::proceed::createby, gear);
                            gear.dismiss();
                        };
                        //auto& world = *data_src;
                        //boss.LISTEN(tier::release, hids::events::mouse::scroll::any, gear, -, (inst_id))
                        //{
                        //   //if (auto data_src = data_src_shadow.lock())
                        //   {
                        //       sptr<vtm::apps> registry_ptr;
                        //       //data_src->SIGNAL(tier::request, vtm::events::list::apps, registry_ptr);
                        //       world.SIGNAL(tier::request, vtm::events::list::apps, registry_ptr);
                        //       auto& app_list = (*registry_ptr)[inst_id];
                        //       if (app_list.size())
                        //       {
                        //           auto deed = boss.bell::protos<tier::release>();
                        //           if (deed == hids::events::mouse::scroll::down.id) // Rotate list forward.
                        //           {
                        //               app_list.push_back(app_list.front());
                        //               app_list.pop_front();
                        //           }
                        //           else // Rotate list backward.
                        //           {
                        //               app_list.push_front(app_list.back());
                        //               app_list.pop_back();
                        //           }
                        //           // Expose window.
                        //           auto& inst = *app_list.back();
                        //           inst.SIGNAL(tier::preview, e2::form::layout::expose, inst);
                        //           auto& area = inst.base::area();
                        //           auto center = area.coor + (area.size / 2);
                        //           gear.owner.SIGNAL(tier::release, e2::form::layout::shift, center);  // Goto to the window.
                        //           gear.pass_kb_focus(inst);
                        //           gear.dismiss();
                        //       }
                        //   }
                        //};
                    });
                if (!state) item_area->depend_on_collection(inst_ptr_list); // Remove not pinned apps, like Info.
                auto block = item_area->attach(ui::fork::ctor(axis::Y));
                auto head_area = block->attach(slot::_1, ui::pads::ctor(dent{ 0,0,0,0 }, dent{ 0,0,1,1 }));
                auto head = head_area->attach(ui::item::ctor(obj_desc, true))
                    ->invoke([&](auto& boss)
                    {
                        auto boss_shadow = ptr::shadow(boss.This());
                        boss.LISTEN(tier::anycast, events::ui::selected, data, -, (inst_id, obj_desc))
                        {
                            auto selected = inst_id == data;
                            boss.set(ansi::fgx(selected ? 0xFF00ff00 : 0x00000000).add(obj_desc));
                            boss.deface();
                        };
                    });
                auto list_pads = block->attach(slot::_2, ui::pads::ctor(dent{ 0,0,0,0 }, dent{ 0,0,0,0 }));
                auto insts = list_pads->attach(ui::list::ctor())
                    ->attach_collection(e2::form::prop::ui::header, inst_ptr_list, app_template);
            }
            return apps;
        };

        auto build = [](text cwd, text v, xmls& config, text patch)
        {
            auto highlight_color = skin::globals().highlight;
            auto action_color    = skin::globals().action;
            auto inactive_color  = skin::globals().inactive;
            auto warning_color   = skin::globals().warning;
            auto danger_color    = skin::globals().danger;
            auto cA = inactive_color;
            auto c3 = highlight_color;
            auto x3 = cell{ c3 }.alpha(0x00);
            auto c6 = action_color;
            auto x6 = cell{ c6 }.alpha(0x00);
            auto c2 = warning_color;
            auto x2 = cell{ c2 }.bga(0x00);
            auto c1 = danger_color;
            auto x1 = cell{ c1 }.bga(0x00);

            auto uibar_min_size = config.take("/config/menu/width/folded",   si32{ 4  });
            auto uibar_max_size = config.take("/config/menu/width/expanded", si32{ 31 });

            auto window = ui::cake::ctor();
            auto my_id = id_t{};

            auto user_info = utf::divide(v, ";");
            if (user_info.size() < 2)
            {
                log("desk: bad window arguments: args=", utf::debase(v));
                return window;
            }
            auto& user_id___view = user_info[0];
            auto& user_name_view = user_info[1];
            auto& menu_selected  = user_info[2];
            log("desk: id: ", user_id___view, ", user name: ", user_name_view);

            if (auto value = utf::to_int(user_id___view)) my_id = value.value();
            else return window;

            if (auto client = bell::getref(my_id))
            {
                auto user_template = [my_id](auto& data_src, auto const& utf8)
                {
                    auto highlight_color = skin::color(tone::highlight);
                    auto c3 = highlight_color;
                    auto x3 = cell{ c3 }.alpha(0x00);

                    auto item_area = ui::pads::ctor(dent{ 1,0,0,1 }, dent{ 0,0,1,0 })
                        ->plugin<pro::fader>(x3, c3, skin::globals().fader_time)
                        ->plugin<pro::notes>(" Connected user ");
                    auto user = item_area->attach(ui::item::ctor(ansi::esc(" &").nil().add(" ")
                        .fgx(data_src->id == my_id ? rgba::color256[whitelt] : 0x00).add(utf8), true));
                    return item_area;
                };
                auto branch_template = [user_template](auto& data_src, auto& usr_list)
                {
                    auto users = ui::list::ctor()
                        ->attach_collection(e2::form::prop::name, *usr_list, user_template);
                    return users;
                };

                window->invoke([uibar_max_size, uibar_min_size, menu_selected](auto& boss) mutable
                {
                    auto current_default  = text{ menu_selected };
                    auto previous_default = text{ menu_selected };
                    boss.LISTEN(tier::release, e2::form::upon::vtree::attached, parent, -, (current_default, previous_default, tokens = subs{}))
                    {
                        parent->SIGNAL(tier::anycast, events::ui::selected, current_default);
                        parent->LISTEN(tier::request, e2::data::changed, data, tokens)
                        {
                            data = current_default;
                        };
                        parent->LISTEN(tier::preview, e2::data::changed, data, tokens)
                        {
                            data = previous_default;
                        };
                        parent->LISTEN(tier::release, e2::data::changed, data, tokens)
                        {
                            boss.SIGNAL(tier::anycast, events::ui::selected, data);
                        };
                        parent->LISTEN(tier::anycast, events::ui::selected, data, tokens)
                        {
                            auto new_default = data;
                            if (current_default != new_default)
                            {
                                previous_default = current_default;
                                current_default = new_default;
                            }
                        };
                        parent->LISTEN(tier::release, e2::form::upon::vtree::detached, p, tokens)
                        {
                            current_default.clear();
                            previous_default.clear();
                            tokens.clear();
                        };
                    };
                });
                auto taskbar_viewport = window->attach(ui::fork::ctor(axis::X))
                    ->invoke([](auto& boss)
                    {
                        boss.LISTEN(tier::anycast, e2::form::prop::viewport, viewport)
                        {
                            viewport = boss.base::area();
                        };
                    });
                auto taskbar = taskbar_viewport->attach(slot::_1, ui::fork::ctor(axis::Y))
                    ->colors(whitedk, 0x60202020)
                    ->plugin<pro::notes>(" LeftDrag to adjust menu width                   \n"
                                         " Ctrl+LeftDrag to adjust folded width            \n"
                                         " RightDrag or scroll wheel to slide menu up/down ")
                    ->plugin<pro::limit>(twod{ uibar_min_size,-1 }, twod{ uibar_min_size,-1 })
                    ->plugin<pro::timer>()
                    ->plugin<pro::acryl>()
                    ->plugin<pro::cache>()
                    ->invoke([&](auto& boss)
                    {
                        boss.mouse.template draggable<hids::buttons::left>(true);
                        auto size_config = ptr::shared(std::pair{ uibar_max_size, uibar_min_size });
                        boss.LISTEN(tier::release, e2::form::drag::pull::_<hids::buttons::left>, gear, -, (size_config))
                        {
                            auto& [uibar_max_size, uibar_min_size] = *size_config;
                            auto& limits = boss.template plugins<pro::limit>();
                            auto lims = limits.get();
                            lims.min.x += gear.delta.get().x;
                            lims.max.x = lims.min.x;
                            gear.meta(hids::anyCtrl) ? uibar_min_size = lims.min.x
                                                     : uibar_max_size = lims.min.x;
                            limits.set(lims.min, lims.max);
                            boss.base::reflow();
                        };
                        boss.LISTEN(tier::release, e2::form::state::mouse, active, -, (size_config))
                        {
                            auto apply = [&, size_config](auto active)
                            {
                                auto& [uibar_max_size, uibar_min_size] = *size_config;
                                auto& limits = boss.template plugins<pro::limit>();
                                auto size = active ? std::max(uibar_max_size, uibar_min_size)
                                                   : uibar_min_size;
                                auto lims = twod{ size,-1 };
                                limits.set(lims, lims);
                                boss.base::reflow();
                                return faux; // One shot call.
                            };
                            auto& timer = boss.template plugins<pro::timer>();
                            timer.pacify(faux);
                            if (active) apply(true);
                            else        timer.actify(faux, skin::globals().menu_timeout, apply);
                        };
                        boss.LISTEN(tier::anycast, e2::form::prop::viewport, viewport, -, (size_config))
                        {
                            auto& [uibar_max_size, uibar_min_size] = *size_config;
                            viewport.coor.x += uibar_min_size;
                            viewport.size.x -= uibar_min_size;
                        };
                    });
                auto apps_users = taskbar->attach(slot::_1, ui::fork::ctor(axis::Y, 0, 100));
                auto applist_area = apps_users->attach(slot::_1, ui::pads::ctor(dent{ 0,0,1,0 }, dent{}))
                    ->attach(ui::cake::ctor());
                auto tasks_scrl = applist_area->attach(ui::rail::ctor(axes::Y_ONLY))
                    ->colors(0x00, 0x00) //todo mouse events passthrough
                    ->invoke([&](auto& boss)
                    {
                        boss.LISTEN(tier::anycast, e2::form::upon::started, parent_ptr)
                        {
                            auto world_ptr = e2::config::creator.param();
                            boss.RISEUP(tier::request, e2::config::creator, world_ptr);
                            if (world_ptr)
                            {
                                auto apps = boss.attach_element(vtm::events::list::apps, world_ptr, apps_template);
                            }
                        };
                    });
                auto users_area = apps_users->attach(slot::_2, ui::fork::ctor(axis::Y));
                auto label_pads = users_area->attach(slot::_1, ui::pads::ctor(dent{ 0,0,1,1 }, dent{ 0,0,0,0 }))
                                            ->plugin<pro::notes>(" List of connected users ");
                auto label_bttn = label_pads->attach(ui::fork::ctor());
                auto label = label_bttn->attach(slot::_1, ui::item::ctor("users", true, faux, true))
                    ->plugin<pro::limit>(twod{ 5,-1 })
                    ->colors(cA.fgc(), cA.bgc());
                auto bttn_area = label_bttn->attach(slot::_2, ui::fork::ctor());
                auto bttn_pads = bttn_area->attach(slot::_2, ui::pads::ctor(dent{ 2,2,0,0 }, dent{ 0,0,1,1 }))
                    ->plugin<pro::fader>(x6, c6, skin::globals().fader_time)
                    ->plugin<pro::notes>(" Show/hide user list ");
                auto bttn = bttn_pads->attach(ui::item::ctor("<", faux));
                auto userlist_area = users_area->attach(slot::_2, ui::pads::ctor())
                    ->plugin<pro::limit>()
                    ->invoke([&](auto& boss)
                    {
                        boss.LISTEN(tier::anycast, e2::form::upon::started, parent_ptr)
                        {
                            auto world_ptr = e2::config::creator.param();
                            boss.RISEUP(tier::request, e2::config::creator, world_ptr);
                            if (world_ptr)
                            {
                                auto users = boss.attach_element(vtm::events::list::usrs, world_ptr, branch_template);
                            }
                        };
                    });
                bttn_pads->invoke([&](auto& boss)
                {
                    auto userlist_area_shadow = ptr::shadow(userlist_area);
                    auto bttn_shadow = ptr::shadow(bttn);
                    boss.LISTEN(tier::release, hids::events::mouse::button::click::left, gear, -, (userlist_area_shadow, bttn_shadow, state = faux))
                    {
                        if (auto bttn = bttn_shadow.lock())
                        if (auto userlist = userlist_area_shadow.lock())
                        {
                            state = !state;
                            bttn->set(state ? ">" : "<");
                            auto& limits = userlist->plugins<pro::limit>();
                            auto lims = limits.get();
                            lims.min.y = lims.max.y = state ? 0 : -1;
                            limits.set(lims, true);
                            userlist->base::reflow();
                        }
                    };
                });
                auto bttns_cake = taskbar->attach(slot::_2, ui::cake::ctor());
                auto bttns_area = bttns_cake->attach(ui::rail::ctor(axes::X_ONLY))
                    ->plugin<pro::limit>(twod{ -1, 3 }, twod{ -1, 3 });
                bttns_cake->attach(app::shared::underlined_hz_scrollbars(bttns_area));
                auto bttns = bttns_area->attach(ui::fork::ctor(axis::X))
                    ->plugin<pro::limit>(twod{ uibar_max_size, 3 }, twod{ -1, 3 });
                auto disconnect_park = bttns->attach(slot::_1, ui::park::ctor())
                    ->plugin<pro::fader>(x2, c2, skin::globals().fader_time)
                    ->plugin<pro::notes>(" Leave current session ")
                    ->invoke([&](auto& boss)
                    {
                        boss.LISTEN(tier::release, hids::events::mouse::button::click::left, gear)
                        {
                            gear.owner.SIGNAL(tier::release, e2::conio::quit, "taskbar: logout by button");
                            gear.dismiss();
                        };
                    });
                auto disconnect_area = disconnect_park->attach(snap::head, snap::center, ui::pads::ctor(dent{ 2,3,1,1 }));
                auto disconnect = disconnect_area->attach(ui::item::ctor("× Disconnect"));
                auto shutdown_park = bttns->attach(slot::_2, ui::park::ctor())
                    ->plugin<pro::fader>(x1, c1, skin::globals().fader_time)
                    ->plugin<pro::notes>(" Disconnect all users and shutdown the server ")
                    ->invoke([&](auto& boss)
                    {
                        boss.LISTEN(tier::release, hids::events::mouse::button::click::left, gear)
                        {
                            boss.SIGNAL(tier::general, e2::shutdown, "desk: server shutdown");
                        };
                    });
                auto shutdown_area = shutdown_park->attach(snap::tail, snap::center, ui::pads::ctor(dent{ 2,3,1,1 }));
                auto shutdown = shutdown_area->attach(ui::item::ctor("× Shutdown"));
            }
            return window;
        };
    }

    app::shared::initialize builder{ app::desk::id, build };
}